/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

let {Cu} = require("chrome");
let Services = require("Services");
let promise = require("sdk/core/promise");
let {Class} = require("sdk/core/heritage");
let {EventTarget} = require("sdk/event/target");
let events = require("sdk/event/core");
let object = require("sdk/util/object");

// Waiting for promise.done() to be added, see bug 851321
function promiseDone(err) {
  console.error(err);
  return promise.reject(err);
}

/**
 * Types: named marshallers/demarshallers.
 *
 * Types provide a 'write' function that takes a js representation and
 * returns a protocol representation, and a "read" function that
 * takes a protocol representation and returns a js representation.
 *
 * The read and write methods are also passed a context object that
 * represent the actor or front requesting the translation.
 *
 * Types are referred to with a typestring.  Basic types are
 * registered by name using addType, and more complex types can
 * be generated by adding detail to the type name.
 */

let types = Object.create(null);
exports.types = types;

let registeredTypes = new Map();
let registeredLifetimes = new Map();

/**
 * Return the type object associated with a given typestring.
 * If passed a type object, it will be returned unchanged.
 *
 * Types can be registered with addType, or can be created on
 * the fly with typestrings.  Examples:
 *
 *   boolean
 *   threadActor
 *   threadActor#detail
 *   array:threadActor
 *   array:array:threadActor#detail
 *
 * @param [typestring|type] type
 *    Either a typestring naming a type or a type object.
 *
 * @returns a type object.
 */
types.getType = function(type) {
  if (!type) {
    return types.Primitive;
  }

  if (typeof(type) !== "string") {
    return type;
  }

  // If already registered, we're done here.
  let reg = registeredTypes.get(type);
  if (reg) return reg;

  // New type, see if it's a collection/lifetime type:
  let sep = type.indexOf(":");
  if (sep >= 0) {
    let collection = type.substring(0, sep);
    let subtype = types.getType(type.substring(sep + 1));

    if (collection === "array") {
      return types.addArrayType(subtype);
    } else if (collection === "nullable") {
      return types.addNullableType(subtype);
    }

    if (registeredLifetimes.has(collection)) {
      return types.addLifetimeType(collection, subtype);
    }

    throw Error("Unknown collection type: " + collection);
  }

  // Not a collection, might be actor detail
  let pieces = type.split("#", 2);
  if (pieces.length > 1) {
    return types.addActorDetail(type, pieces[0], pieces[1]);
  }

  // Might be a lazily-loaded type
  if (type === "longstring") {
    require("devtools/server/actors/string");
    return registeredTypes.get("longstring");
  }

  throw Error("Unknown type: " + type);
}

/**
 * Don't allow undefined when writing primitive types to packets.  If
 * you want to allow undefined, use a nullable type.
 */
function identityWrite(v) {
  if (v === undefined) {
    throw Error("undefined passed where a value is required");
  }
  return v;
}

/**
 * Add a type to the type system.
 *
 * When registering a type, you can provide `read` and `write` methods.
 *
 * The `read` method will be passed a JS object value from the JSON
 * packet and must return a native representation.  The `write` method will
 * be passed a native representation and should provide a JSONable value.
 *
 * These methods will both be passed a context.  The context is the object
 * performing or servicing the request - on the server side it will be
 * an Actor, on the client side it will be a Front.
 *
 * @param typestring name
 *    Name to register
 * @param object typeObject
 *    An object whose properties will be stored in the type, including
 *    the `read` and `write` methods.
 * @param object options
 *    Can specify `thawed` to prevent the type from being frozen.
 *
 * @returns a type object that can be used in protocol definitions.
 */
types.addType = function(name, typeObject={}, options={}) {
  if (registeredTypes.has(name)) {
    throw Error("Type '" + name + "' already exists.");
  }

  let type = object.merge({
    name: name,
    primitive: !(typeObject.read || typeObject.write),
    read: identityWrite,
    write: identityWrite
  }, typeObject);

  registeredTypes.set(name, type);

  if (!options.thawed) {
    Object.freeze(type);
  }

  return type;
};

/**
 * Add an array type to the type system.
 *
 * getType() will call this function if provided an "array:<type>"
 * typestring.
 *
 * @param type subtype
 *    The subtype to be held by the array.
 */
types.addArrayType = function(subtype) {
  subtype = types.getType(subtype);

  let name = "array:" + subtype.name;

  // Arrays of primitive types are primitive types themselves.
  if (subtype.primitive) {
    return types.addType(name);
  }
  return types.addType(name, {
    read: (v, ctx) => [subtype.read(i, ctx) for (i of v)],
    write: (v, ctx) => [subtype.write(i, ctx) for (i of v)]
  });
};

/**
 * Add a dict type to the type system.  This allows you to serialize
 * a JS object that contains non-primitive subtypes.
 *
 * Properties of the value that aren't included in the specializations
 * will be serialized as primitive values.
 *
 * @param object specializations
 *    A dict of property names => type
 */
types.addDictType = function(name, specializations) {
  return types.addType(name, {
    read: (v, ctx) => {
      let ret = {};
      for (let prop in v) {
        if (prop in specializations) {
          ret[prop] = types.getType(specializations[prop]).read(v[prop], ctx);
        } else {
          ret[prop] = v[prop];
        }
      }
      return ret;
    },

    write: (v, ctx) => {
      let ret = {};
      for (let prop in v) {
        if (prop in specializations) {
          ret[prop] = types.getType(specializations[prop]).write(v[prop], ctx);
        } else {
          ret[prop] = v[prop];
        }
      }
      return ret;
    }
  })
}

/**
 * Register an actor type with the type system.
 *
 * Types are marshalled differently when communicating server->client
 * than they are when communicating client->server.  The server needs
 * to provide useful information to the client, so uses the actor's
 * `form` method to get a json representation of the actor.  When
 * making a request from the client we only need the actor ID string.
 *
 * This function can be called before the associated actor has been
 * constructed, but the read and write methods won't work until
 * the associated addActorImpl or addActorFront methods have been
 * called during actor/front construction.
 *
 * @param string name
 *    The typestring to register.
 */
types.addActorType = function(name) {
  let type = types.addType(name, {
    _actor: true,
    read: (v, ctx, detail) => {
      // If we're reading a request on the server side, just
      // find the actor registered with this actorID.
      if (ctx instanceof Actor) {
        return ctx.conn.getActor(v);
      }

      // Reading a response on the client side, check for an
      // existing front on the connection, and create the front
      // if it isn't found.
      let actorID = typeof(v) === "string" ? v : v.actor;
      let front = ctx.conn.getActor(actorID);
      if (front) {
        front.form(v, detail, ctx);
      } else {
        front = new type.frontClass(ctx.conn, v, detail, ctx)
        front.actorID = actorID;
        ctx.marshallPool().manage(front);
      }
      return front;
    },
    write: (v, ctx, detail) => {
      // If returning a response from the server side, make sure
      // the actor is added to a parent object and return its form.
      if (v instanceof Actor) {
        if (!v.actorID) {
          ctx.marshallPool().manage(v);
        }
        return v.form(detail);
      }

      // Writing a request from the client side, just send the actor id.
      return v.actorID;
    },
  }, {
    // We usually freeze types, but actor types are updated when clients are
    // created, so don't freeze yet.
    thawed: true
  });
  return type;
}

types.addNullableType = function(subtype) {
  subtype = types.getType(subtype);
  return types.addType("nullable:" + subtype.name, {
    read: (value, ctx) => {
      if (value == null) {
        return value;
      }
      return subtype.read(value, ctx);
    },
    write: (value, ctx) => {
      if (value == null) {
        return value;
      }
      return subtype.write(value, ctx);
    }
  });
}

/**
 * Register an actor detail type.  This is just like an actor type, but
 * will pass a detail hint to the actor's form method during serialization/
 * deserialization.
 *
 * This is called by getType() when passed an 'actorType#detail' string.
 *
 * @param string name
 *   The typestring to register this type as.
 * @param type actorType
 *   The actor type you'll be detailing.
 * @param string detail
 *   The detail to pass.
 */
types.addActorDetail = function(name, actorType, detail) {
  actorType = types.getType(actorType);
  if (!actorType._actor) {
    throw Error("Details only apply to actor types, tried to add detail '" + detail + "'' to " + actorType.name + "\n");
  }
  return types.addType(name, {
    _actor: true,
    read: (v, ctx) => actorType.read(v, ctx, detail),
    write: (v, ctx) => actorType.write(v, ctx, detail)
  });
}

/**
 * Register an actor lifetime.  This lets the type system find a parent
 * actor that differs from the actor fulfilling the request.
 *
 * @param string name
 *    The lifetime name to use in typestrings.
 * @param string prop
 *    The property of the actor that holds the parent that should be used.
 */
types.addLifetime = function(name, prop) {
  if (registeredLifetimes.has(name)) {
    throw Error("Lifetime '" + name + "' already registered.");
  }
  registeredLifetimes.set(name, prop);
}

/**
 * Register a lifetime type.  This creates an actor type tied to the given
 * lifetime.
 *
 * This is called by getType() when passed a '<lifetimeType>:<actorType>'
 * typestring.
 *
 * @param string lifetime
 *    A lifetime string previously regisered with addLifetime()
 * @param type subtype
 *    An actor type
 */
types.addLifetimeType = function(lifetime, subtype) {
  subtype = types.getType(subtype);
  if (!subtype._actor) {
    throw Error("Lifetimes only apply to actor types, tried to apply lifetime '" + lifetime + "'' to " + subtype.name);
  }
  let prop = registeredLifetimes.get(lifetime);
  return types.addType(lifetime + ":" + subtype.name, {
    read: (value, ctx) => subtype.read(value, ctx[prop]),
    write: (value, ctx) => subtype.write(value, ctx[prop])
  })
}

// Add a few named primitive types.
types.Primitive = types.addType("primitive");
types.String = types.addType("string");
types.Number = types.addType("number");
types.Boolean = types.addType("boolean");
types.JSON = types.addType("json");

/**
 * Request/Response templates and generation
 *
 * Request packets are specified as json templates with
 * Arg and Option placeholders where arguments should be
 * placed.
 *
 * Reponse packets are also specified as json templates,
 * with a RetVal placeholder where the return value should be
 * placed.
 */

/**
 * Placeholder for simple arguments.
 *
 * @param number index
 *    The argument index to place at this position.
 * @param type type
 *    The argument should be marshalled as this type.
 * @constructor
 */
let Arg = Class({
  initialize: function(index, type) {
    this.index = index;
    this.type = types.getType(type);
  },

  write: function(arg, ctx) {
    return this.type.write(arg, ctx);
  },

  read: function(v, ctx, outArgs) {
    outArgs[this.index] = this.type.read(v, ctx);
  }
});
exports.Arg = Arg;

/**
 * Placeholder for an options argument value that should be hoisted
 * into the packet.
 *
 * If provided in a method specification:
 *
 *   { optionArg: Option(1)}
 *
 * Then arguments[1].optionArg will be placed in the packet in this
 * value's place.
 *
 * @param number index
 *    The argument index of the options value.
 * @param type type
 *    The argument should be marshalled as this type.
 * @constructor
 */
let Option = Class({
  extends: Arg,
  initialize: function(index, type) {
    Arg.prototype.initialize.call(this, index, type)
  },

  write: function(arg, ctx, name) {
    if (!arg) {
      return undefined;
    }
    let v = arg[name] || undefined;
    if (v === undefined) {
      return undefined;
    }
    return this.type.write(v, ctx);
  },
  read: function(v, ctx, outArgs, name) {
    if (outArgs[this.index] === undefined) {
      outArgs[this.index] = {};
    }
    if (v === undefined) {
      return;
    }
    outArgs[this.index][name] = this.type.read(v, ctx);
  }
});

exports.Option = Option;

/**
 * Placeholder for return values in a response template.
 *
 * @param type type
 *    The return value should be marshalled as this type.
 */
let RetVal = Class({
  initialize: function(type) {
    this.type = types.getType(type);
  },

  write: function(v, ctx) {
    return this.type.write(v, ctx);
  },

  read: function(v, ctx) {
    return this.type.read(v, ctx);
  }
});

exports.RetVal = RetVal;

/* Template handling functions */

/**
 * Get the value at a given path, or undefined if not found.
 */
function getPath(obj, path) {
  for (let name of path) {
    if (!(name in obj)) {
      return undefined;
    }
    obj = obj[name];
  }
  return obj;
}

/**
 * Find Placeholders in the template and save them along with their
 * paths.
 */
function findPlaceholders(template, constructor, path=[], placeholders=[]) {
  if (!template || typeof(template) != "object") {
    return placeholders;
  }

  if (template instanceof constructor) {
    placeholders.push({ placeholder: template, path: [p for (p of path)] });
    return placeholders;
  }

  for (let name in template) {
    path.push(name);
    findPlaceholders(template[name], constructor, path, placeholders);
    path.pop();
  }

  return placeholders;
}


/**
 * Manages a request template.
 *
 * @param object template
 *    The request template.
 * @construcor
 */
let Request = Class({
  initialize: function(template={}) {
    this.type = template.type;
    this.template = template;
    this.args = findPlaceholders(template, Arg);
  },

  /**
   * Write a request.
   *
   * @param array fnArgs
   *    The function arguments to place in the request.
   * @param object ctx
   *    The object making the request.
   * @returns a request packet.
   */
  write: function(fnArgs, ctx) {
    let str = JSON.stringify(this.template, (key, value) => {
      if (value instanceof Arg) {
        return value.write(fnArgs[value.index], ctx, key);
      }
      return value;
    });
    return JSON.parse(str);
  },

  /**
   * Read a request.
   *
   * @param object packet
   *    The request packet.
   * @param object ctx
   *    The object making the request.
   * @returns an arguments array
   */
  read: function(packet, ctx) {
    let fnArgs = [];
    for (let templateArg of this.args) {
      let arg = templateArg.placeholder;
      let path = templateArg.path;
      let name = path[path.length - 1];
      arg.read(getPath(packet, path), ctx, fnArgs, name);
    }
    return fnArgs;
  },
});

/**
 * Manages a response template.
 *
 * @param object template
 *    The response template.
 * @construcor
 */
let Response = Class({
  initialize: function(template={}) {
    this.template = template;
    let placeholders = findPlaceholders(template, RetVal);
    if (placeholders.length > 1) {
      throw Error("More than one RetVal specified in response");
    }
    let placeholder = placeholders.shift();
    if (placeholder) {
      this.retVal = placeholder.placeholder;
      this.path = placeholder.path;
    }
  },

  /**
   * Write a response for the given return value.
   *
   * @param val ret
   *    The return value.
   * @param object ctx
   *    The object writing the response.
   */
  write: function(ret, ctx) {
    return JSON.parse(JSON.stringify(this.template, function(key, value) {
      if (value instanceof RetVal) {
        return value.write(ret, ctx);
      }
      return value;
    }));
  },

  /**
   * Read a return value from the given response.
   *
   * @param object packet
   *    The response packet.
   * @param object ctx
   *    The object reading the response.
   */
  read: function(packet, ctx) {
    if (!this.retVal) {
      return undefined;
    }
    let v = getPath(packet, this.path);
    return this.retVal.read(v, ctx);
  }
});

/**
 * Actor and Front implementations
 */

/**
 * A protocol object that can manage the lifetime of other protocol
 * objects.
 */
let Pool = Class({
  extends: EventTarget,

  /**
   * Pools are used on both sides of the connection to help coordinate
   * lifetimes.
   *
   * @param optional conn
   *   Either a DebuggerServerConnection or a DebuggerClient.  Must have
   *   addActorPool, removeActorPool, and poolFor.
   *   conn can be null if the subclass provides a conn property.
   * @constructor
   */
  initialize: function(conn) {
    if (conn) {
      this.conn = conn;
    }
  },

  /**
   * Return the parent pool for this client.
   */
  parent: function() { return this.conn.poolFor(this.actorID) },

  /**
   * Override this if you want actors returned by this actor
   * to belong to a different actor by default.
   */
  marshallPool: function() { return this; },

  /**
   * Pool is the base class for all actors, even leaf nodes.
   * If the child map is actually referenced, go ahead and create
   * the stuff needed by the pool.
   */
  __poolMap: null,
  get _poolMap() {
    if (this.__poolMap) return this.__poolMap;
    this.__poolMap = new Map();
    this.conn.addActorPool(this);
    return this.__poolMap;
  },

  /**
   * Add an actor as a child of this pool.
   */
  manage: function(actor) {
    if (!actor.actorID) {
      actor.actorID = this.conn.allocID(actor.actorPrefix || actor.typeName);
    }

    this._poolMap.set(actor.actorID, actor);
    return actor;
  },

  /**
   * Remove an actor as a child of this pool.
   */
  unmanage: function(actor) {
    this.__poolMap.delete(actor.actorID);
  },

  // true if the given actor ID exists in the pool.
  has: function(actorID) this.__poolMap && this._poolMap.has(actorID),

  // The actor for a given actor id stored in this pool
  actor: function(actorID) this.__poolMap ? this._poolMap.get(actorID) : null,

  // Same as actor, should update debugger connection to use 'actor'
  // and then remove this.
  get: function(actorID) this.__poolMap ? this._poolMap.get(actorID) : null,

  // True if this pool has no children.
  isEmpty: function() !this.__poolMap || this._poolMap.size == 0,

  /**
   * Destroy this item, removing it from a parent if it has one,
   * and destroying all children if necessary.
   */
  destroy: function() {
    let parent = this.parent();
    if (parent) {
      parent.unmanage(this);
    }
    if (!this.__poolMap) {
      return;
    }
    for (let actor of this.__poolMap.values()) {
      // Self-owned actors are ok, but don't need destroying twice.
      if (actor === this) {
        continue;
      }
      let destroy = actor.destroy;
      if (destroy) {
        // Disconnect destroy while we're destroying in case of (misbehaving)
        // circular ownership.
        actor.destroy = null;
        destroy.call(actor);
        actor.destroy = destroy;
      }
    };
    this.conn.removeActorPool(this, true);
    this.__poolMap.clear();
    this.__poolMap = null;
  },

  /**
   * For getting along with the debugger server pools, should be removable
   * eventually.
   */
  cleanup: function() {
    this.destroy();
  }
});
exports.Pool = Pool;

/**
 * An actor in the actor tree.
 */
let Actor = Class({
  extends: Pool,

  // Will contain the actor's ID
  actorID: null,

  /**
   * Initialize an actor.
   *
   * @param optional conn
   *   Either a DebuggerServerConnection or a DebuggerClient.  Must have
   *   addActorPool, removeActorPool, and poolFor.
   *   conn can be null if the subclass provides a conn property.
   * @constructor
   */
  initialize: function(conn) {
    Pool.prototype.initialize.call(this, conn);

    // Forward events to the connection.
    if (this._actorSpec && this._actorSpec.events) {
      for (let key of this._actorSpec.events.keys()) {
        let name = key;
        let sendEvent = this._sendEvent.bind(this, name)
        this.on(name, (...args) => {
          sendEvent.apply(null, args);
        });
      }
    }
  },

  _sendEvent: function(name, ...args) {
    if (!this._actorSpec.events.has(name)) {
      // It's ok to emit events that don't go over the wire.
      return;
    }
    let request = this._actorSpec.events.get(name);
    let packet = request.write(args, this);
    packet.from = packet.from || this.actorID;
    this.conn.send(packet);
  },

  destroy: function() {
    Pool.prototype.destroy.call(this);
    this.actorID = null;
  },

  /**
   * Override this method in subclasses to serialize the actor.
   * @param [optional] string hint
   *   Optional string to customize the form.
   * @returns A jsonable object.
   */
  form: function(hint) {
    return { actor: this.actorID }
  },

  writeError: function(err) {
    console.error(err);
    if (err.stack) {
      dump(err.stack);
    }
    this.conn.send({
      from: this.actorID,
      error: "unknownError",
      message: err.toString()
    });
  },

  _queueResponse: function(create) {
    let pending = this._pendingResponse || promise.resolve(null);
    let response = create(pending);
    this._pendingResponse = response;
  }
});
exports.Actor = Actor;

/**
 * Tags a prtotype method as an actor method implementation.
 *
 * @param function fn
 *    The implementation function, will be returned.
 * @param spec
 *    The method specification, with the following (optional) properties:
 *      request (object): a request template.
 *      response (object): a response template.
 *      oneway (bool): 'true' if no response should be sent.
 *      telemetry (string): Telemetry probe ID for measuring completion time.
 */
exports.method = function(fn, spec={}) {
  fn._methodSpec = Object.freeze(spec);
  if (spec.request) Object.freeze(spec.request);
  if (spec.response) Object.freeze(spec.response);
  return fn;
}

/**
 * Process an actor definition from its prototype and generate
 * request handlers.
 */
let actorProto = function(actorProto) {
  if (actorProto._actorSpec) {
    throw new Error("actorProto called twice on the same actor prototype!");
  }

  let protoSpec = {
    methods: [],
  };

  // Find method specifications attached to prototype properties.
  for (let name of Object.getOwnPropertyNames(actorProto)) {
    let desc = Object.getOwnPropertyDescriptor(actorProto, name);
    if (!desc.value) {
      continue;
    }

    if (desc.value._methodSpec) {
      let frozenSpec = desc.value._methodSpec;
      let spec = {};
      spec.name = frozenSpec.name || name;
      spec.request = Request(object.merge({type: spec.name}, frozenSpec.request || undefined));
      spec.response = Response(frozenSpec.response || undefined);
      spec.telemetry = frozenSpec.telemetry;
      spec.release = frozenSpec.release;
      spec.oneway = frozenSpec.oneway;

      protoSpec.methods.push(spec);
    }
  }

  // Find event specifications
  if (actorProto.events) {
    protoSpec.events = new Map();
    for (let name in actorProto.events) {
      let eventRequest = actorProto.events[name];
      Object.freeze(eventRequest);
      protoSpec.events.set(name, Request(object.merge({type: name}, eventRequest)));
    }
  }

  // Generate request handlers for each method definition
  actorProto.requestTypes = Object.create(null);
  protoSpec.methods.forEach(spec => {
    let handler = function(packet, conn) {
      try {
        let args = spec.request.read(packet, this);

        let ret = this[spec.name].apply(this, args);

        if (spec.oneway) {
          // No need to send a response.
          return;
        }

        let sendReturn = (ret) => {
          let response = spec.response.write(ret, this);
          response.from = this.actorID;
          // If spec.release has been specified, destroy the object.
          if (spec.release) {
            try {
              this.destroy();
            } catch(e) {
              this.writeError(e);
              return;
            }
          }

          conn.send(response);
        };

        this._queueResponse(p => {
          return p
            .then(() => ret)
            .then(sendReturn)
            .then(null, this.writeError.bind(this));
        })
      } catch(e) {
        this._queueResponse(p => {
          return p.then(() => this.writeError(e));
        });
      }
    };

    actorProto.requestTypes[spec.request.type] = handler;
  });

  actorProto._actorSpec = protoSpec;
  return actorProto;
}

/**
 * Create an actor class for the given actor prototype.
 *
 * @param object proto
 *    The object prototype.  Must have a 'typeName' property,
 *    should have method definitions, can have event definitions.
 */
exports.ActorClass = function(proto) {
  if (!proto.typeName) {
    throw Error("Actor prototype must have a typeName member.");
  }
  proto.extends = Actor;
  if (!registeredTypes.has(proto.typeName)) {
    types.addActorType(proto.typeName);
  }
  return Class(actorProto(proto));
};

/**
 * Base class for client-side actor fronts.
 */
let Front = Class({
  extends: Pool,

  actorID: null,

  /**
   * The base class for client-side actor fronts.
   *
   * @param optional conn
   *   Either a DebuggerServerConnection or a DebuggerClient.  Must have
   *   addActorPool, removeActorPool, and poolFor.
   *   conn can be null if the subclass provides a conn property.
   * @param optional form
   *   The json form provided by the server.
   * @constructor
   */
  initialize: function(conn=null, form=null, detail=null, context=null) {
    Pool.prototype.initialize.call(this, conn);
    this._requests = [];
    if (form) {
      this.actorID = form.actor;
      this.form(form, detail, context);
    }
  },

  destroy: function() {
    // Reject all outstanding requests, they won't make sense after
    // the front is destroyed.
    while (this._requests && this._requests.length > 0) {
      let deferred = this._requests.shift();
      deferred.reject(new Error("Connection closed"));
    }
    Pool.prototype.destroy.call(this);
    this.actorID = null;
  },

  /**
   * @returns a promise that will resolve to the actorID this front
   * represents.
   */
  actor: function() { return promise.resolve(this.actorID) },

  toString: function() { return "[Front for " + this.typeName + "/" + this.actorID + "]" },

  /**
   * Update the actor from its representation.
   * Subclasses should override this.
   */
  form: function(form) {},

  /**
   * Send a packet on the connection.
   */
  send: function(packet) {
    if (packet.to) {
      this.conn._transport.send(packet);
    } else {
      this.actor().then(actorID => {
        packet.to = actorID;
        this.conn._transport.send(packet);
      });
    }
  },

  /**
   * Send a two-way request on the connection.
   */
  request: function(packet) {
    let deferred = promise.defer();
    this._requests.push(deferred);
    this.send(packet);
    return deferred.promise;
  },

  /**
   * Handler for incoming packets from the client's actor.
   */
  onPacket: function(packet) {
    // Pick off event packets
    if (this._clientSpec.events && this._clientSpec.events.has(packet.type)) {
      let event = this._clientSpec.events.get(packet.type);
      let args = event.request.read(packet, this);
      if (event.pre) {
        event.pre.forEach((pre) => pre.apply(this, args));
      }
      events.emit.apply(null, [this, event.name].concat(args));
      return;
    }

    // Remaining packets must be responses.
    if (this._requests.length === 0) {
      let msg = "Unexpected packet " + this.actorID + ", " + JSON.stringify(packet);
      let err = Error(msg);
      console.error(err);
      throw err;
    }

    let deferred = this._requests.shift();
    if (packet.error) {
      deferred.reject(packet.error);
    } else {
      deferred.resolve(packet);
    }
  }
});
exports.Front = Front;

/**
 * A method tagged with preEvent will be called after recieving a packet
 * for that event, and before the front emits the event.
 */
exports.preEvent = function(eventName, fn) {
  fn._preEvent = eventName;
  return fn;
}

/**
 * Mark a method as a custom front implementation, replacing the generated
 * front method.
 *
 * @param function fn
 *    The front implementation, will be returned.
 * @param object options
 *    Options object:
 *      impl (string): If provided, the generated front method will be
 *        stored as this property on the prototype.
 */
exports.custom = function(fn, options={}) {
  fn._customFront = options;
  return fn;
}

function prototypeOf(obj) {
  return typeof(obj) === "function" ? obj.prototype : obj;
}

/**
 * Process a front definition from its prototype and generate
 * request methods.
 */
let frontProto = function(proto) {
  let actorType = prototypeOf(proto.actorType);
  if (proto._actorSpec) {
    throw new Error("frontProto called twice on the same front prototype!");
  }
  proto._actorSpec = actorType._actorSpec;
  proto.typeName = actorType.typeName;

  // Generate request methods.
  let methods = proto._actorSpec.methods;
  methods.forEach(spec => {
    let name = spec.name;

    // If there's already a property by this name in the front, it must
    // be a custom front method.
    if (name in proto) {
      let custom = proto[spec.name]._customFront;
      if (custom === undefined) {
        throw Error("Existing method for " + spec.name + " not marked customFront while processing " + actorType.typeName + ".");
      }
      // If the user doesn't need the impl don't generate it.
      if (!custom.impl) {
        return;
      }
      name = custom.impl;
    }

    proto[name] = function(...args) {
      let histogram, startTime;
      if (spec.telemetry) {
        if (spec.oneway) {
          // That just doesn't make sense.
          throw Error("Telemetry specified for a oneway request");
        }
        let transportType = this.conn.localTransport
          ? "LOCAL_"
          : "REMOTE_";
        let histogramId = "DEVTOOLS_DEBUGGER_RDP_"
          + transportType + spec.telemetry + "_MS";
        try {
          histogram = Services.telemetry.getHistogramById(histogramId);
          startTime = new Date();
        } catch(ex) {
          // XXX: Is this expected in xpcshell tests?
          console.error(ex);
          spec.telemetry = false;
        }
      }

      let packet = spec.request.write(args, this);
      if (spec.oneway) {
        // Fire-and-forget oneway packets.
        this.send(packet);
        return undefined;
      }

      return this.request(packet).then(response => {
        let ret = spec.response.read(response, this);

        if (histogram) {
          histogram.add(+new Date - startTime);
        }

        return ret;
      }).then(null, promiseDone);
    }

    // Release methods should call the destroy function on return.
    if (spec.release) {
      let fn = proto[name];
      proto[name] = function(...args) {
        return fn.apply(this, args).then(result => {
          this.destroy();
          return result;
        })
      }
    }
  });


  // Process event specifications
  proto._clientSpec = {};

  let events = proto._actorSpec.events;
  if (events) {
    // This actor has events, scan the prototype for preEvent handlers...
    let preHandlers = new Map();
    for (let name of Object.getOwnPropertyNames(proto)) {
      let desc = Object.getOwnPropertyDescriptor(proto, name);
      if (!desc.value) {
        continue;
      }
      if (desc.value._preEvent) {
        let preEvent = desc.value._preEvent;
        if (!events.has(preEvent)) {
          throw Error("preEvent for event that doesn't exist: " + preEvent);
        }
        let handlers = preHandlers.get(preEvent);
        if (!handlers) {
          handlers = [];
          preHandlers.set(preEvent, handlers);
        }
        handlers.push(desc.value);
      }
    }

    proto._clientSpec.events = new Map();

    for (let [name, request] of events) {
      proto._clientSpec.events.set(request.type, {
        name: name,
        request: request,
        pre: preHandlers.get(name)
      });
    }
  }
  return proto;
}

/**
 * Create a front class for the given actor class, with the given prototype.
 *
 * @param ActorClass actorType
 *    The actor class you're creating a front for.
 * @param object proto
 *    The object prototype.  Must have a 'typeName' property,
 *    should have method definitions, can have event definitions.
 */
exports.FrontClass = function(actorType, proto) {
  proto.actorType = actorType;
  proto.extends = Front;
  let cls = Class(frontProto(proto));
  registeredTypes.get(cls.prototype.typeName).frontClass = cls;
  return cls;
}
