#!/usr/bin/env python
# header.py - Generate C++ header files from IDL.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import sys, os, xpidl, makeutils

def strip_end(text, suffix):
    if not text.endswith(suffix):
        return text
    return text[:-len(suffix)]

def findIDL(includePath, interfaceFileName):
    for d in includePath:
        # Not os.path.join: we need a forward slash even on Windows because
        # this filename ends up in makedepend output.
        path = d + '/' + interfaceFileName
        if os.path.exists(path):
            return path
    raise BaseException("No IDL file found for interface %s "
                        "in include path %r"
                        % (interfaceFileName, includePath))

eventFileNameToIdl = {};

def loadIDL(parser, includePath, filename):
    global eventFileNameToIdl
    if filename in eventFileNameToIdl:
        return eventFileNameToIdl[filename]

    idlFile = findIDL(includePath, filename)
    if not idlFile in makeutils.dependencies:
        makeutils.dependencies.append(idlFile)
    idl = p.parse(open(idlFile).read(), idlFile)
    idl.resolve(includePath, p)
    eventFileNameToIdl[filename] = idl
    return idl

def loadEventIDL(parser, includePath, eventname):
    eventidl = ("nsIDOM%s.idl" % eventname)
    return loadIDL(parser, includePath, eventidl)

class Configuration:
    def __init__(self, filename):
        config = {}
        execfile(filename, config)
        self.simple_events = config.get('simple_events', [])
        self.special_includes = config.get('special_includes', [])
        self.exclude_automatic_type_include = config.get('exclude_automatic_type_include', [])
        self.xpidl_to_native = config.get('xpidl_to_native', [])

def readConfigFile(filename):
    return Configuration(filename)

def firstCap(str):
    return str[0].upper() + str[1:]

def getBaseName(iface):
    return ("%s" % iface.base[6:])

def print_header_file(fd, conf):
    fd.write("#ifndef _gen_mozilla_idl_generated_events_h_\n"
             "#define _gen_mozilla_idl_generated_events_h_\n\n")
    fd.write("/* THIS FILE IS AUTOGENERATED - DO NOT EDIT */\n")
    fd.write("#include \"nscore.h\"\n")
    fd.write("class nsIDOMEvent;\n")
    fd.write("class nsPresContext;\n")
    fd.write("namespace mozilla {\n");
    fd.write("class WidgetEvent;\n")
    fd.write("namespace dom {\n");
    fd.write("class EventTarget;\n")
    fd.write("}\n");
    fd.write("}\n\n");
    for e in conf.simple_events:
        fd.write("nsresult\n")
        fd.write("NS_NewDOM%s(nsIDOMEvent** aInstance, " % e)
        fd.write("mozilla::dom::EventTarget* aOwner, ")
        fd.write("nsPresContext* aPresContext, mozilla::WidgetEvent* aEvent);\n")

    fd.write("\n#endif\n")

def print_classes_file(fd, conf):
    fd.write("#ifndef _gen_mozilla_idl_generated_event_declarations_h_\n")
    fd.write("#define _gen_mozilla_idl_generated_event_declarations_h_\n\n")

    fd.write("#include \"mozilla/dom/Event.h\"\n");
    includes = []
    for s in conf.special_includes:
        if not s in includes:
            includes.append(strip_end(s, ".h"))

    for e in conf.simple_events:
        if not e in includes:
            includes.append(("nsIDOM%s" % e))

    attrnames = []
    for e in conf.simple_events:
        idl = loadEventIDL(p, options.incdirs, e)
        collect_names_and_non_primitive_attribute_types(idl, attrnames, includes)

    for c in includes:
      if not c in conf.exclude_automatic_type_include:
            fd.write("#include \"%s.h\"\n" % c)

    for e in conf.simple_events:
        fd.write('#include "mozilla/dom/%sBinding.h"\n' % e);

    fd.write("namespace mozilla {\n")
    fd.write("namespace dom {\n")
    for e in conf.simple_events:
        idl = loadEventIDL(p, options.incdirs, e)
        for pr in idl.productions:
            if pr.kind == 'interface':
                print_class_declaration(e, pr, fd, conf)
    fd.write("} // namespace dom\n")
    fd.write("} // namespace mozilla\n\n")
    fd.write("#endif\n");

def print_class_declaration(eventname, iface, fd, conf):
    classname = ("%s" % eventname)
    basename = getBaseName(iface)
    attributes = []
    ccattributes = []
    for member in iface.members:
        if isinstance(member, xpidl.Attribute):
            attributes.append(member)
            if (member.realtype.nativeType('in').endswith('*')):
                ccattributes.append(member);

    baseinterfaces = []
    baseiface = iface.idl.getName(iface.base, iface.location)
    while baseiface.name != "nsIDOMEvent":
        baseinterfaces.append(baseiface)
        baseiface = baseiface.idl.getName(baseiface.base, baseiface.location)
    baseinterfaces.reverse()

    allattributes = []
    for baseiface in baseinterfaces:
        for member in baseiface.members:
            if isinstance(member, xpidl.Attribute):
                allattributes.append(member)
    allattributes.extend(attributes);

    fd.write("\nclass %s MOZ_FINAL : public %s, public %s\n" % (classname, basename, iface.name))
    fd.write("{\n")
    fd.write("public:\n")
    fd.write("  %s(mozilla::dom::EventTarget* aOwner, " % classname)
    fd.write("nsPresContext* aPresContext = nullptr, mozilla::WidgetEvent* aEvent = nullptr);\n");
    fd.write("  virtual ~%s();\n\n" % classname)
    fd.write("  NS_DECL_ISUPPORTS_INHERITED\n")
    fd.write("  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(%s, %s)\n" % (classname, basename))
    fd.write("  NS_FORWARD_TO_EVENT\n")

    for baseiface in baseinterfaces:
        baseimpl = ("%s" % baseiface.name[6:])
        fd.write("  NS_FORWARD_%s(%s::)\n" % (baseiface.name.upper(), baseimpl))

    fd.write("  NS_DECL_%s\n" % iface.name.upper())

    hasVariant = False
    for a in allattributes:
        if a.type == "nsIVariant":
            hasVariant = True
            break;
    fd.write("  static already_AddRefed<%s> Constructor(const GlobalObject& aGlobal, " % eventname)
    if hasVariant:
        fd.write("JSContext* aCx, ")
    fd.write("const nsAString& aType, ")
    fd.write("const %sInit& aParam, " % eventname)
    fd.write("ErrorResult& aRv);\n\n")

    fd.write("  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE\n")
    fd.write("  {\n")
    fd.write("    return mozilla::dom::%sBinding::Wrap(aCx, this);\n" % eventname)
    fd.write("  }\n\n")

    for a in attributes:
        """xpidl methods take care of string member variables!"""
        firstCapName = firstCap(a.name)
        cleanNativeType = a.realtype.nativeType('in').strip('* ')
        if a.realtype.nativeType('in').count("nsAString"):
            continue
        elif a.realtype.nativeType('in').count("nsIVariant"):
            fd.write("  void Get%s(JSContext* aCx, JS::MutableHandle<JS::Value> aRetval, ErrorResult& aRv);\n\n" % firstCapName);
        elif a.realtype.nativeType('in').endswith('*'):
            fd.write("  already_AddRefed<%s> Get%s()\n" % (xpidl_to_native(cleanNativeType, conf), firstCapName))
            fd.write("  {\n");
            fd.write("    nsCOMPtr<%s> %s = do_QueryInterface(m%s);\n" % (xpidl_to_canonical(cleanNativeType, conf), a.name, firstCapName))
            fd.write("    return %s.forget().downcast<%s>();\n" % (a.name, xpidl_to_native(cleanNativeType, conf)))
            fd.write("  }\n\n");
        else:
            fd.write("  %s %s()\n" % (cleanNativeType, firstCapName))
            fd.write("  {\n");
            fd.write("    return m%s;\n" % firstCapName)
            fd.write("  }\n\n");

    fd.write("  void ")
    fd.write("Init%s(" % eventname)
    if hasVariant:
        fd.write("JSContext* aCx, ")
    fd.write("const nsAString& aType, bool aCanBubble, bool aCancelable")
    for a in allattributes:
        writeNativeAttributeParams(fd, a, conf)
    fd.write(", ErrorResult& aRv);\n\n")

    fd.write("protected:\n")
    for a in attributes:
        fd.write("  %s\n" % attributeVariableTypeAndName(a))
    fd.write("};\n")

def collect_names_and_non_primitive_attribute_types(idl, attrnames, forwards):
    for p in idl.productions:
        if p.kind == 'interface' or p.kind == 'dictionary':
            interfaces = []
            base = p.base
            baseiface = p
            while base is not None:
                baseiface = baseiface.idl.getName(baseiface.base, baseiface.location)    
                interfaces.append(baseiface)
                base = baseiface.base

            interfaces.reverse()
            interfaces.append(p)

            for iface in interfaces:
                collect_names_and_non_primitive_attribute_types_from_interface(iface, attrnames, forwards)

def collect_names_and_non_primitive_attribute_types_from_interface(iface, attrnames, forwards):
    for member in iface.members:
        if isinstance(member, xpidl.Attribute):
            if not member.name in attrnames:
                attrnames.append(member.name)
            if member.realtype.nativeType('in').endswith('*'):
                t = member.realtype.nativeType('in').strip('* ')
                if not t in forwards:
                    forwards.append(t)

def print_cpp(idl, fd, conf, eventname):
    for p in idl.productions:
        if p.kind == 'interface':
            write_cpp(eventname, p, fd, conf)

def print_cpp_file(fd, conf):
    fd.write("/* THIS FILE IS AUTOGENERATED - DO NOT EDIT */\n\n")
    fd.write('#include "GeneratedEventClasses.h"\n')
    fd.write('#include "xpcprivate.h"\n');
    fd.write('#include "mozilla/dom/Event.h"\n');
    fd.write('#include "mozilla/dom/EventTarget.h"\n');

    for e in conf.simple_events:
        idl = loadEventIDL(p, options.incdirs, e)
        print_cpp(idl, fd, conf, e)

def init_value(attribute):
    realtype = attribute.realtype.nativeType('in')
    realtype = realtype.strip(' ')
    if realtype.endswith('*'):
        return "nullptr"
    if realtype == "bool":
        return "false"
    if realtype.count("nsAString"):
        return ""
    if realtype.count("nsACString"):
        return ""
    if realtype.count("JS::Value"):
      raise BaseException("JS::Value not supported in simple events!")
    return "0"

def attributeVariableTypeAndName(a):
    if a.realtype.nativeType('in').endswith('*'):
        l = ["nsCOMPtr<%s> m%s;" % (a.realtype.nativeType('in').strip('* '),
                   firstCap(a.name))]
    elif a.realtype.nativeType('in').count("nsAString"):
        l = ["nsString m%s;" % firstCap(a.name)]
    elif a.realtype.nativeType('in').count("nsACString"):
        l = ["nsCString m%s;" % firstCap(a.name)]
    else:
        l = ["%sm%s;" % (a.realtype.nativeType('in'),
                       firstCap(a.name))]
    return ", ".join(l)

def writeAttributeGetter(fd, classname, a):
    fd.write("NS_IMETHODIMP\n")
    fd.write("%s::Get%s(" % (classname, firstCap(a.name)))
    if a.realtype.nativeType('in').endswith('*'):
        fd.write("%s** a%s" % (a.realtype.nativeType('in').strip('* '), firstCap(a.name)))
    elif a.realtype.nativeType('in').count("nsAString"):
        fd.write("nsAString& a%s" % firstCap(a.name))
    elif a.realtype.nativeType('in').count("nsACString"):
        fd.write("nsACString& a%s" % firstCap(a.name))
    else:
        fd.write("%s*a%s" % (a.realtype.nativeType('in'), firstCap(a.name)))
    fd.write(")\n");
    fd.write("{\n");
    if a.realtype.nativeType('in').endswith('*'):
        fd.write("  NS_IF_ADDREF(*a%s = m%s);\n" % (firstCap(a.name), firstCap(a.name)))
    elif a.realtype.nativeType('in').count("nsAString"):
        fd.write("  a%s = m%s;\n" % (firstCap(a.name), firstCap(a.name)))
    elif a.realtype.nativeType('in').count("nsACString"):
        fd.write("  a%s = m%s;\n" % (firstCap(a.name), firstCap(a.name)))
    else:
        fd.write("  *a%s = %s();\n" % (firstCap(a.name), firstCap(a.name)))
    fd.write("  return NS_OK;\n");
    fd.write("}\n\n");
    if a.realtype.nativeType('in').count("nsIVariant"):
        fd.write("void\n")
        fd.write("%s::Get%s(JSContext* aCx, JS::MutableHandle<JS::Value> aRetval, ErrorResult& aRv)\n" % (classname, firstCap(a.name)))
        fd.write("{\n")
        fd.write("  nsresult rv = NS_ERROR_UNEXPECTED;\n")
        fd.write("  if (!m%s) {\n" % firstCap(a.name))
        fd.write("    aRetval.setNull();\n")
        fd.write("  } else if (!XPCVariant::VariantDataToJS(m%s, &rv, aRetval)) {\n" % (firstCap(a.name)))
        fd.write("    aRv.Throw(NS_ERROR_FAILURE);\n")
        fd.write("  }\n")
        fd.write("}\n\n")

def writeAttributeParams(fd, a):
    if a.realtype.nativeType('in').endswith('*'):
        fd.write(", %s* a%s" % (a.realtype.nativeType('in').strip('* '), firstCap(a.name)))
    elif a.realtype.nativeType('in').count("nsAString"):
        fd.write(", const nsAString& a%s" % firstCap(a.name))
    elif a.realtype.nativeType('in').count("nsACString"):
        fd.write(", const nsACString& a%s" % firstCap(a.name))
    else:
        fd.write(", %s a%s" % (a.realtype.nativeType('in'), firstCap(a.name)))

def writeNativeAttributeParams(fd, a, conf):
    if a.type == "nsIVariant":
        fd.write(", JS::Value a%s" % firstCap(a.name));
    elif a.realtype.nativeType('in').endswith('*'):
        fd.write(", %s* a%s" % (xpidl_to_native(a.realtype.nativeType('in').strip('* '), conf), firstCap(a.name)))
    elif a.realtype.nativeType('in').count("nsAString"):
        fd.write(", const nsAString& a%s" % firstCap(a.name))
    elif a.realtype.nativeType('in').count("nsACString"):
        fd.write(", const nsACString& a%s" % firstCap(a.name))
    else:
        fd.write(", %s a%s" % (a.realtype.nativeType('in'), firstCap(a.name)))

def write_cpp(eventname, iface, fd, conf):
    classname = ("%s" % eventname)
    basename = getBaseName(iface)
    attributes = []
    ccattributes = []
    for member in iface.members:
        if isinstance(member, xpidl.Attribute):
            attributes.append(member)
            if (member.realtype.nativeType('in').endswith('*')):
                ccattributes.append(member);

    baseinterfaces = []
    baseiface = iface.idl.getName(iface.base, iface.location)
    while baseiface.name != "nsIDOMEvent":
        baseinterfaces.append(baseiface)
        baseiface = baseiface.idl.getName(baseiface.base, baseiface.location)
    baseinterfaces.reverse()

    baseattributes = []
    for baseiface in baseinterfaces:
        for member in baseiface.members:
            if isinstance(member, xpidl.Attribute):
                baseattributes.append(member)

    allattributes = []
    allattributes.extend(baseattributes);
    allattributes.extend(attributes);

    fd.write("namespace mozilla {\n")
    fd.write("namespace dom {\n\n")

    fd.write("%s::%s(mozilla::dom::EventTarget* aOwner, " % (classname, classname))
    fd.write("nsPresContext* aPresContext, mozilla::WidgetEvent* aEvent)\n");
    fd.write(": %s(aOwner, aPresContext, aEvent)" % basename)
    for a in attributes:
        fd.write(",\n  m%s(%s)" % (firstCap(a.name), init_value(a)))
    fd.write("\n{\n")
    fd.write("}\n\n")

    fd.write("%s::~%s() {}\n\n" % (classname, classname))

    fd.write("NS_IMPL_CYCLE_COLLECTION_CLASS(%s)\n" % (classname))
    fd.write("NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(%s, %s)\n" % (classname, basename))
    for c in ccattributes:
        fd.write("  NS_IMPL_CYCLE_COLLECTION_UNLINK(m%s)\n" % firstCap(c.name))
    fd.write("NS_IMPL_CYCLE_COLLECTION_UNLINK_END\n\n");
    fd.write("NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(%s, %s)\n" % (classname, basename))
    for c in ccattributes:
        fd.write("  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(m%s)\n" % firstCap(c.name))
    fd.write("NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END\n\n");

    fd.write("NS_IMPL_ADDREF_INHERITED(%s, %s)\n" % (classname, basename))
    fd.write("NS_IMPL_RELEASE_INHERITED(%s, %s)\n\n" % (classname, basename))

    fd.write("NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(%s)\n" % classname)
    fd.write("  NS_INTERFACE_MAP_ENTRY(nsIDOM%s)\n" % eventname)
    fd.write("NS_INTERFACE_MAP_END_INHERITING(%s)\n\n" % basename)

    hasVariant = False
    for a in allattributes:
        if a.type == "nsIVariant":
            hasVariant = True
            break;

    fd.write("already_AddRefed<%s>\n" % eventname)
    fd.write("%s::Constructor(const GlobalObject& aGlobal, " % eventname)
    if hasVariant:
        fd.write("JSContext* aCx, ");
    fd.write("const nsAString& aType, ")
    fd.write("const %sInit& aParam, " % eventname)
    fd.write("ErrorResult& aRv)\n")
    fd.write("{\n")
    fd.write("  nsCOMPtr<mozilla::dom::EventTarget> t = do_QueryInterface(aGlobal.GetAsSupports());\n")
    fd.write("  nsRefPtr<%s> e = new %s(t, nullptr, nullptr);\n" % (eventname, eventname))
    fd.write("  bool trusted = e->Init(t);\n")
    fd.write("  e->Init%s(" % eventname)
    if hasVariant:
        fd.write("aCx, ");
    fd.write("aType, aParam.mBubbles, aParam.mCancelable")
    for a in allattributes:
        fd.write(", aParam.m%s" % firstCap(a.name))
    fd.write(", aRv);\n")
    fd.write("  e->SetTrusted(trusted);\n")
    fd.write("  return e.forget();\n")
    fd.write("}\n\n")

    fd.write("NS_IMETHODIMP\n")
    fd.write("%s::Init%s(" % (classname, eventname))
    fd.write("const nsAString& aType, bool aCanBubble, bool aCancelable")
    for a in allattributes:
        writeAttributeParams(fd, a)
    fd.write(")\n{\n")
    fd.write("  nsresult rv = %s::Init%s(aType, aCanBubble, aCancelable" % (basename, ("%s" % iface.base[6:])))
    for a in baseattributes:
      fd.write(", a%s" % firstCap(a.name))
    fd.write(");\n");
    fd.write("  NS_ENSURE_SUCCESS(rv, rv);\n")
    for a in attributes:
        if a.realtype.nativeType("in").count("nsAString"):
            fd.write("  if (!m%s.Assign(a%s, fallible_t())) {\n" % (firstCap(a.name), firstCap(a.name)))
            fd.write("    return NS_ERROR_OUT_OF_MEMORY;\n")
            fd.write("  }\n")
        else:
            fd.write("  m%s = a%s;\n" % (firstCap(a.name), firstCap(a.name)))
    fd.write("  return NS_OK;\n")
    fd.write("}\n\n")

    fd.write("void\n")
    fd.write("%s::Init%s(" % (classname, eventname))
    if hasVariant:
        fd.write("JSContext* aCx, ")
    fd.write("const nsAString& aType, bool aCanBubble, bool aCancelable")
    for a in allattributes:
        writeNativeAttributeParams(fd, a, conf)
    fd.write(", ErrorResult& aRv")
    fd.write(")\n")
    fd.write("{\n");
    for a in allattributes:
        if a.type == "nsIVariant":
            fd.write("  nsCOMPtr<nsIVariant> %s = dont_AddRef(XPCVariant::newVariant(aCx, a%s));\n" % (a.name, firstCap(a.name)))
            fd.write("  if (!%s) {\n" % a.name)
            fd.write("    aRv.Throw(NS_ERROR_FAILURE);\n")
            fd.write("    return;\n")
            fd.write("  }\n")
        elif a.realtype.nativeType('in').endswith('*'):
            xpidl_t = a.realtype.nativeType('in').strip('* ')
            native_t = xpidl_to_native(xpidl_t, conf)
            if xpidl_t != native_t:
                fd.write("  nsCOMPtr<%s> %s = do_QueryInterface(static_cast<%s*>(a%s));\n" % (xpidl_t, a.name, xpidl_to_canonical(xpidl_t, conf), firstCap(a.name)))
    fd.write("  aRv = Init%s(" % classname);
    fd.write("aType, aCanBubble, aCancelable")
    for a in allattributes:
        if a.realtype.nativeType('in').endswith('*'):
            xpidl_t = a.realtype.nativeType('in').strip('* ')
            native_t = xpidl_to_native(xpidl_t, conf)
            if xpidl_t != native_t or a.type == "nsIVariant":
                fd.write(", %s" % a.name)
                continue
        fd.write(", a%s" % firstCap(a.name))
    fd.write(");\n}\n\n");

    for a in attributes:
        writeAttributeGetter(fd, classname, a)

    fd.write("} // namespace dom\n")
    fd.write("} // namespace mozilla\n\n")

    fd.write("nsresult\n")
    fd.write("NS_NewDOM%s(nsIDOMEvent** aInstance, "  % eventname)
    fd.write("mozilla::dom::EventTarget* aOwner, nsPresContext* aPresContext = nullptr, mozilla::WidgetEvent* aEvent = nullptr)\n")
    fd.write("{\n")
    fd.write("  mozilla::dom::%s* it = new mozilla::dom::%s(aOwner, aPresContext, aEvent);\n" % (classname, classname))
    fd.write("  NS_ADDREF(it);\n")
    fd.write("  *aInstance = static_cast<mozilla::dom::Event*>(it);\n")
    fd.write("  return NS_OK;\n");
    fd.write("}\n\n")

def toWebIDLType(attribute, inType=False, onlyInterface=False):
    if attribute.type == "nsIVariant":
        return "any";
    if attribute.type == "nsISupports":
        return "%s%s" % (attribute.type, "" if onlyInterface else "?")
    if attribute.type.count("nsIDOM"):
        return "%s%s" % (attribute.type[6:], "" if onlyInterface else "?")
    if attribute.type.count("nsI"):
        return "%s%s" % (attribute.type[3:], "" if onlyInterface else "?")
    if attribute.realtype.nativeType('in').endswith('*') or attribute.realtype.nativeType('in').count("nsAString"):
        return "%s%s" % (attribute.type, "" if onlyInterface else "?")
    return attribute.type

def write_webidl(eventname, iface, fd, conf, idl):
    basename = ("%s" % iface.base[6:])
    attributes = []
    ccattributes = []
    consts = []
    hasInit = False
    initMethod = "init%s" % eventname;
    for member in iface.members:
        if isinstance(member, xpidl.Attribute):
            attributes.append(member)
            if (member.realtype.nativeType('in').endswith('*')):
                ccattributes.append(member);
        elif isinstance(member, xpidl.Method) and member.name == initMethod:
            if not member.noscript and not member.notxpcom:
                hasInit = True
        elif isinstance(member, xpidl.ConstMember):
            consts.append(member);
        else:
            raise BaseException("Unsupported idl member %s::%s" % (eventname, member.name))

    baseinterfaces = []
    baseiface = iface.idl.getName(iface.base, iface.location)
    while baseiface.name != "nsIDOMEvent":
        baseinterfaces.append(baseiface)
        baseiface = baseiface.idl.getName(baseiface.base, baseiface.location)
    baseinterfaces.reverse()

    allattributes = []
    for baseiface in baseinterfaces:
        for member in baseiface.members:
            if isinstance(member, xpidl.Attribute):
                allattributes.append(member)
    allattributes.extend(attributes)

    fd.write(
"""/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */
""")

    neededInterfaces = []
    for a in attributes:
        if a.realtype.nativeType('in').endswith('*'):
            nativeType = a.realtype.nativeType('in').strip('* ')
            mappingForWebIDL = False
            for xp in conf.xpidl_to_native:
                if xp[0] == nativeType:
                    mappingForWebIDL = True
                    break;
            if not mappingForWebIDL:
                webidlType = toWebIDLType(a, False, True)
                if (not webidlType in neededInterfaces and webidlType != "any"):
                    neededInterfaces.append(webidlType);

    for i in neededInterfaces:
        fd.write("interface %s;\n" % i)

    fd.write("\n");
    fd.write("[Constructor(DOMString type, optional %sInit eventInitDict), HeaderFile=\"GeneratedEventClasses.h\"]\n" % eventname);
    fd.write("interface %s : %s\n" % (eventname, basename))
    fd.write("{\n")

    for c in consts:
        fd.write("  const %s %s = %s;\n" % (c.type, c.name, c.getValue()))
    if len(consts):
        fd.write("\n")

    for a in attributes:
        if a.realtype.nativeType('in').count("nsIVariant"):
            fd.write("  [Throws]\n")
            fd.write("  readonly attribute %s %s;\n" % (toWebIDLType(a), a.name))
        elif a.realtype.nativeType('in').endswith('*') or a.realtype.nativeType('in').count("nsAString"):
            fd.write("  readonly attribute %s %s;\n" % (toWebIDLType(a), a.name))
        else:
            fd.write("  readonly attribute %s %s;\n" % (a.type, a.name))
    if hasInit:
        fd.write("\n  [Throws]\n")
        m = "  void %s(" % initMethod
        fd.write(m)
        indent = "".join(" " for i in range(len(m)))
        indent = ",\n%s" % indent
        fd.write("DOMString type")
        fd.write(indent);
        fd.write("boolean canBubble")
        fd.write(indent);
        fd.write("boolean cancelable")
        for a in baseattributes + attributes:
            fd.write(indent);
            fd.write("%s %s" % (toWebIDLType(a, True), a.name))
        fd.write(");\n");
    fd.write("};\n\n")

    dname = "%sInit" % eventname
    for p in idl.productions:
        if p.kind == "dictionary" and p.name == dname:
            fd.write("dictionary %s : %sInit\n" % (dname, basename))
            fd.write("{\n")
            # We want to keep the same ordering what interface has.
            for ifaceattribute in attributes:
                for member in p.members:
                    if member.name == ifaceattribute.name:
                        a = member
                        if a.realtype.nativeType('in').endswith('*'):
                            fd.write("  %s %s = null;\n" % (toWebIDLType(a, True), a.name))
                        elif a.realtype.nativeType('in').count("nsAString"):
                            if a.defvalue is None:
                                if a.nullable:
                                    fd.write("  %s? %s = null;\n" % (a.type, a.name))
                                else:
                                    fd.write("  %s %s = \"\";\n" % (a.type, a.name))
                            else:
                                if a.nullable:
                                    fd.write("  %s? %s = \"%s\";\n" % (a.type, a.name, a.defvalue))
                                else:
                                    fd.write("  %s %s = \"%s\";\n" % (a.type, a.name, a.defvalue))
                        else:
                            if a.defvalue is None:
                                if a.type == "boolean":
                                    fd.write("  %s %s = false;\n" % (a.type, a.name))
                                else:
                                    fd.write("  %s %s = 0;\n" % (a.type, a.name))
                            # Infinity is not supported by all the types, but
                            # WebIDL parser will then complain about the wrong values.
                            elif a.defvalue == "Infinity":
                                fd.write("  unrestricted %s %s = Infinity;\n" % (a.type, a.name))
                            elif a.defvalue == "-Infinity":
                                fd.write("  unrestricted %s %s = -Infinity;\n" % (a.type, a.name))
                            else:
                                fd.write("  %s %s = %s;\n" % (a.type, a.name, a.defvalue))
                    continue
            fd.write("};\n")
            return

    # There is no dictionary defined in the .idl file. Generate one based on
    # the interface.
    fd.write("dictionary %s : %sInit\n" % (dname, basename))
    fd.write("{\n")
    for a in attributes:
        if a.realtype.nativeType('in').endswith('*'):
            fd.write("  %s %s = null;\n" % (toWebIDLType(a, True), a.name))
        elif a.realtype.nativeType('in').count("nsAString"):
            fd.write("  %s? %s = \"\";\n" % (a.type, a.name))
        elif a.type == "boolean":
            fd.write("  %s %s = false;\n" % (a.type, a.name))
        else:
            fd.write("  %s %s = 0;\n" % (a.type, a.name))
    fd.write("};\n")

def print_webidl_file(idl, fd, conf, eventname):
    for p in idl.productions:
        if p.kind == 'interface':
            write_webidl(eventname, p, fd, conf, idl)

def xpidl_to_native(xpidl, conf):
    for x in conf.xpidl_to_native:
        if x[0] == xpidl:
            return x[1]
    return xpidl

def xpidl_to_canonical(xpidl, conf):
    for x in conf.xpidl_to_native:
        if x[0] == xpidl:
            return x[2]
    return xpidl

def native_to_xpidl(native, conf):
    for x in conf.xpidl_to_native:
        if x[1] == native:
            return x[0]
    return native

def print_webidl_files(webidlDir, conf):
    for e in conf.simple_events:
        idl = loadEventIDL(p, options.incdirs, e)
        webidl = "%s/%s.webidl" % (webidlDir, e)
        if not os.path.exists(webidl):
            fd = open(webidl, 'w')
            print_webidl_file(idl, fd, conf, e)
            fd.close();

if __name__ == '__main__':
    from optparse import OptionParser
    o = OptionParser(usage="usage: %prog [options] configfile")
    o.add_option('-I', action='append', dest='incdirs', default=['.'],
                 help="Directory to search for imported files")
    o.add_option('-o', "--stub-output",
                 type='string', dest='stub_output', default=None,
                 help="Quick stub C++ source output file", metavar="FILE")
    o.add_option('--header-output', type='string', default=None,
                 help="Quick stub header output file", metavar="FILE")
    o.add_option('--makedepend-output', type='string', default=None,
                 help="gnumake dependencies output file", metavar="FILE")
    o.add_option('--cachedir', dest='cachedir', default=None,
                 help="Directory in which to cache lex/parse tables.")
    o.add_option('--class-declarations', type='string', default=None,
                 help="Class declarations", metavar="FILE")
    o.add_option('--webidltarget', dest='webidltarget', default=None,
                 help="Directory in which to store generated WebIDL files.")
    (options, filenames) = o.parse_args()
    if len(filenames) != 1:
        o.error("Exactly one config filename is needed.")
    filename = filenames[0]

    if options.cachedir is not None:
        if not os.path.isdir(options.cachedir):
            os.mkdir(options.cachedir)
        sys.path.append(options.cachedir)

    # Instantiate the parser.
    p = xpidl.IDLParser(outputdir=options.cachedir)

    conf = readConfigFile(filename)

    if options.header_output is not None:
        outfd = open(options.header_output, 'w')
        print_header_file(outfd, conf)
        outfd.close()
    if options.class_declarations is not None:
        outfd = open(options.class_declarations, 'w')
        print_classes_file(outfd, conf)
        outfd.close()
    if options.stub_output is not None:
        makeutils.targets.append(options.stub_output)
        outfd = open(options.stub_output, 'w')
        print_cpp_file(outfd, conf)
        outfd.close()
        if options.makedepend_output is not None:
            makeutils.writeMakeDependOutput(options.makedepend_output)

    if options.webidltarget is not None:
        print_webidl_files(options.webidltarget, conf)

