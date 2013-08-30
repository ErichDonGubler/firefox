/* -*- Mode: Javascript; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

"use strict";

loadRelativeToScript('utility.js');
loadRelativeToScript('annotations.js');
loadRelativeToScript('CFG.js');

var subclasses = {};
var superclasses = {};
var classFunctions = {};

function addClassEntry(index, name, other)
{
    if (!(name in index)) {
        index[name] = [other];
        return;
    }

    for (var entry of index[name]) {
        if (entry == other)
            return;
    }

    index[name].push(other);
}

// CSU is "Class/Struct/Union"
function processCSU(csuName, csu)
{
    if (!("FunctionField" in csu))
        return;
    for (var field of csu.FunctionField) {
        if (1 in field.Field) {
            var superclass = field.Field[1].Type.Name;
            var subclass = field.Field[1].FieldCSU.Type.Name;
            assert(subclass == csuName);
            addClassEntry(subclasses, superclass, subclass);
            addClassEntry(superclasses, subclass, superclass);
        }
        if ("Variable" in field) {
            // Note: not dealing with overloading correctly.
            var name = field.Variable.Name[0];
            var key = csuName + ":" + field.Field[0].Name[0];
            if (!(key in classFunctions))
                classFunctions[key] = [];
            classFunctions[key].push(name);
        }
    }
}

function findVirtualFunctions(csu, field)
{
    var worklist = [csu];

    // Virtual call targets on subclasses of nsISupports may be incomplete,
    // if the interface is scriptable. Just treat all indirect calls on
    // nsISupports objects as potentially GC'ing, except AddRef/Release
    // which should never enter the JS engine (even when calling dtors).
    while (worklist.length) {
        var csu = worklist.pop();
        if (csu == "nsISupports") {
            if (field == "AddRef" || field == "Release")
                return [];
            return null;
        }
        if (csu in superclasses) {
            for (var superclass of superclasses[csu])
                worklist.push(superclass);
        }
    }

    var functions = [];
    var worklist = [csu];

    while (worklist.length) {
        var csu = worklist.pop();
        var key = csu + ":" + field;

        if (key in classFunctions) {
            for (var name of classFunctions[key])
                functions.push(name);
        }

        if (csu in subclasses) {
            for (var subclass of subclasses[csu])
                worklist.push(subclass);
        }
    }

    return functions;
}

var memoized = {};
var memoizedCount = 0;

function memo(name)
{
    if (!(name in memoized)) {
        memoizedCount++;
        memoized[name] = "" + memoizedCount;
        print("#" + memoizedCount + " " + name);
    }
    return memoized[name];
}

var seenCallees = null;
var seenSuppressedCallees = null;

// Return a list of all callees that the given edge might be a call to. Each
// one is represented by an object with a 'kind' field that is one of
// ('direct', 'field', 'indirect', 'unknown').
function getCallees(edge)
{
    if (edge.Kind != "Call")
        return [];

    var callee = edge.Exp[0];
    var callees = [];
    if (callee.Kind == "Var") {
        assert(callee.Variable.Kind == "Func");
        var origName = callee.Variable.Name[0];
        var names = [ origName, otherDestructorName(origName) ];
        for (var name of names) {
            if (name)
                callees.push({'kind': 'direct', 'name': name});
        }
    } else {
        assert(callee.Kind == "Drf");
        if (callee.Exp[0].Kind == "Fld") {
            var field = callee.Exp[0].Field;
            var fieldName = field.Name[0];
            var csuName = field.FieldCSU.Type.Name;
            var functions = null;
            if ("FieldInstanceFunction" in field)
                functions = findVirtualFunctions(csuName, fieldName);
            if (functions) {
                // Known set of virtual call targets.
                for (var name of functions)
                    callees.push({'kind': "direct", 'name': name});
            } else {
                // Unknown set of call targets. Non-virtual field call,
                // or virtual call on an nsISupports object.
                callees.push({'kind': "field", 'csu': csuName, 'field': fieldName});
            }
        } else if (callee.Exp[0].Kind == "Var") {
            // indirect call through a variable.
            callees.push({'kind': "indirect", 'variable': callee.Exp[0].Variable.Name[0]});
        } else {
            // unknown call target.
            callees.push({'kind': "unknown"});
        }
    }

    return callees;
}

var lastline;
function printOnce(line)
{
    if (line != lastline) {
        print(line);
        lastline = line;
    }
}

function processBody(caller, body)
{
    if (!('PEdge' in body))
        return;

    lastline = null;
    for (var edge of body.PEdge) {
        if (edge.Kind != "Call")
            continue;
        var suppressText = "";
        var seen = seenCallees;
        if (edge.Index[0] in body.suppressed) {
            suppressText = "SUPPRESS_GC ";
            seen = seenSuppressedCallees;
        }
        var prologue = suppressText + memo(caller) + " ";
        for (var callee of getCallees(edge)) {
            if (callee.kind == 'direct') {
                if (!(callee.name in seen)) {
                    seen[name] = true;
                    printOnce("D " + prologue + memo(callee.name));
                }
            } else if (callee.kind == 'field') {
                var { csu, field } = callee;
                printOnce("F " + prologue + "CLASS " + csu + " FIELD " + field);
            } else if (callee.kind == 'indirect') {
                printOnce("I " + prologue + "VARIABLE " + callee.variable);
            } else if (callee.kind == 'unknown') {
                printOnce("I " + prologue + "VARIABLE UNKNOWN");
            } else {
                printErr("invalid " + callee.kind + " callee");
                debugger;
            }
        }
    }
}

var callgraph = {};

var xdb = xdbLibrary();
xdb.open("src_comp.xdb");

var minStream = xdb.min_data_stream();
var maxStream = xdb.max_data_stream();

for (var csuIndex = minStream; csuIndex <= maxStream; csuIndex++) {
    var csu = xdb.read_key(csuIndex);
    var data = xdb.read_entry(csu);
    var json = JSON.parse(data.readString());
    processCSU(csu.readString(), json[0]);

    xdb.free_string(csu);
    xdb.free_string(data);
}

xdb.open("src_body.xdb");

var minStream = xdb.min_data_stream();
var maxStream = xdb.max_data_stream();

for (var nameIndex = minStream; nameIndex <= maxStream; nameIndex++) {
    var name = xdb.read_key(nameIndex);
    var data = xdb.read_entry(name);
    functionBodies = JSON.parse(data.readString());
    for (var body of functionBodies)
        body.suppressed = [];
    for (var body of functionBodies) {
        for (var [pbody, id] of allRAIIGuardedCallPoints(body, isSuppressConstructor))
            pbody.suppressed[id] = true;
    }

    seenCallees = {};
    seenSuppressedCallees = {};

    for (var body of functionBodies)
        processBody(name.readString(), body);

    xdb.free_string(name);
    xdb.free_string(data);
}
