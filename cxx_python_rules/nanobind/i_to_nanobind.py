#!/usr/bin/env python
"""Convert SWIG .i files (written with the old swig_rules macros) to nanobind
binding code.

    i_to_nanobind.py --namespace Skeleton --header-dir lib --out-dir OUT a.i b.i ...

All the .i files of a module are read together, so class hierarchies and
%template instances can span files. For each a.i this writes OUT/a_nb.cc
with a function ``void init_a(nb::module_& m)``, and it writes
OUT/<prefix>_init.cc (``<prefix>_init_all(m)``, calling the init functions in
%import order; call it from NB_MODULE) and OUT/<prefix>_opaque.h
(NB_MAKE_OPAQUE for each %template vector, which every binding file must
include).

The output is meant to be maintained by hand afterwards, so it is written
as ordinary nanobind code (cxx_python_rules/nanobind/README.md, "Rules for
binding code"): member pointers, nb::overload_cast for overloads, "name"_a
arguments, a cxx_python_rules_nb::docs helper per class, "using namespace"
for the library's namespace, and lambdas only where they do something.
Check the result: keyword names come from the .i parameter names, and
.none() is only added with --allow-none.

The .i files are the specification of the python API, so the output
follows them:

* class + bases -> nb::class_. nanobind allows one bound base, and never a
  virtual base (claude_notes/implementation_plan.md §2.1). The first base
  is used if it isn't GenericObject and isn't virtual in the C++ header;
  the methods of the other bases are bound on the class directly.
* constructors -> nb::new_ returning std::make_shared (so a python
  constructed object has a C++ control block, decision 6), or nb::init
  for a director class (nanobind constructs the trampoline for python
  subclasses).
* methods -> .def / .def_static with a member pointer (nb::overload_cast
  if the name is overloaded). A lambda only where needed: parameters typed
  as a reference to a class that is a secondary or virtual base somewhere
  are taken as a shared_ptr (which the cxx_python_rules caster converts
  through the type registry), OUTPUT/FORCE_COPY/stream arguments, and the
  director dispatch for virtual functions of a director class.
* the %python_attribute variants -> .def_prop_ro / _rw (the _virtual
  variant keeps SWIG's _v_<name> method, which python overrides).
* %python_attribute_std_array -> property using to_std_array.
* %pickle_serialization(_dir), %pickle_init, %pickle_vector -> the
  cxx_python_rules_nb helpers.
* %feature("director") -> a trampoline class with NB_OVERRIDE for every
  virtual function declared in the .i (for the class and its bases);
  CXX_PYTHON_RULES_NB_OVERRIDE_REF for one whose only argument is a
  reference to a bound class (NB_OVERRIDE would pass python a copy).
* %skeleton_director_serialization -> CXX_PYTHON_RULES_NB_DIRECTOR_SERIALIZATION.
* %template(Name) std::vector<...> -> nb::bind_vector; %template of a
  class template -> that class with the template argument filled in.
* OUTPUT/OUTPUT1/OUTPUT2 arguments -> returned in a tuple; FORCE_COPY ->
  a copy; std::istream&/std::ostream& -> a python file object.
* print_to_string -> also __str__.
* free functions -> m.def.

Anything else (%extend, %pythoncode other than the attribute macros,
%typemap, ...) is copied into the output as a "HAND CONVERT" comment.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path


# ---------------------------------------------------------------------------
# Parsing helpers
# ---------------------------------------------------------------------------


def match_brace(s: str, i: int, open_c: str = "{", close_c: str = "}") -> int:
    """s[i] is open_c; return index just past the matching close_c."""
    depth = 0
    j = i
    while j < len(s):
        c = s[j]
        if c == open_c:
            depth += 1
        elif c == close_c:
            depth -= 1
            if depth == 0:
                return j + 1
        elif c == '"':
            j = s.index('"', j + 1)
        j += 1
    raise ValueError("unbalanced " + open_c)


def split_top(s: str, sep: str = ",") -> list[str]:
    """Split on sep at bracket depth 0 (<>, (), {}, [])."""
    res, depth, cur = [], 0, ""
    for c in s:
        if c in "<({[":
            depth += 1
        elif c in ">)}]":
            depth -= 1
        if c == sep and depth == 0:
            res.append(cur)
            cur = ""
        else:
            cur += c
    if cur.strip():
        res.append(cur)
    return [r.strip() for r in res]


def strip_comments(s: str) -> str:
    s = re.sub(r"/\*.*?\*/", "", s, flags=re.S)
    return re.sub(r"//[^\n]*", "", s)


def norm_type(t: str) -> str:
    t = t.replace("boost::shared_ptr", "std::shared_ptr")
    t = t.replace("boost::optional", "std::optional")
    t = re.sub(r"\s+", " ", t).strip()
    t = re.sub(r"\s*([<>,&*])\s*", r"\1", t)
    t = t.replace(",", ", ")
    t = re.sub(r"(\w)([&*])", r"\1 \2", t)
    t = re.sub(r">([&*])", r"> \1", t)
    return t


# ---------------------------------------------------------------------------
# Model
# ---------------------------------------------------------------------------


@dataclass
class Param:
    type: str
    name: str
    default: str | None = None


@dataclass
class Func:
    name: str
    ret: str
    params: list[Param]
    const: bool = False
    static: bool = False
    virtual: bool = False
    pure: bool = False
    pyname: str | None = None


@dataclass
class Attr:
    kind: str  # macro name
    pyname: str
    cppname: str
    type: str


@dataclass
class ClassInfo:
    name: str
    module: str
    bases: list[str]
    body: str
    template: str | None = None  # template parameter name
    ctors: list[Func] = field(default_factory=list)
    protected_default_ctor: bool = False
    methods: list[Func] = field(default_factory=list)
    attrs: list[Attr] = field(default_factory=list)
    pickle: list[str] = field(default_factory=list)  # helper calls
    hand: list[str] = field(default_factory=list)
    director: bool = False
    director_serialization: bool = False
    nodefaultctor: bool = False
    pyname: str = ""
    cpp: str = ""


@dataclass
class Module:
    name: str
    path: Path
    includes: list[str] = field(default_factory=list)
    imports: list[str] = field(default_factory=list)
    items: list[tuple[str, object]] = field(default_factory=list)  # order
    hand: list[str] = field(default_factory=list)
    functions: list[Func] = field(default_factory=list)


# Types a setter takes by value (swig_python_attribute.i declares
# "void NAME(const TYPE& V)", but the C++ usually has "void NAME(TYPE V)" for these).
SCALARS = {
    "bool",
    "int",
    "unsigned int",
    "long",
    "float",
    "double",
    "short",
    "char",
    "std::size_t",
    "size_t",
}

FUNC_RE = re.compile(
    r"^(?P<pre>(?:(?:static|virtual|inline|explicit)\s+)*)"
    r"(?P<ret>.*?)\b(?P<name>~?\w+)\s*\((?P<params>.*)\)\s*"
    r"(?P<const>const)?\s*(?:override)?\s*(?P<pure>=\s*0)?\s*$",
    re.S,
)


def parse_params(s: str) -> list[Param]:
    res = []
    if s.strip() in ("", "void"):
        return res
    for i, p in enumerate(split_top(s)):
        default = None
        if "=" in p:
            p, default = p.split("=", 1)
            default = default.strip()
        p = p.strip()
        m = re.match(r"^(.*?)(\w+)$", p)
        if m and m.group(1).strip() and not m.group(1).strip().endswith("::"):
            typ, name = m.group(1), m.group(2)
        else:
            typ, name = p, f"arg{i}"
        res.append(Param(norm_type(typ), name, default))
    return res


def parse_func(stmt: str) -> Func | None:
    m = FUNC_RE.match(stmt.strip())
    if not m:
        return None
    pre = m.group("pre")
    return Func(
        name=m.group("name"),
        ret=norm_type(m.group("ret")),
        params=parse_params(m.group("params")),
        const=bool(m.group("const")),
        static="static" in pre,
        virtual="virtual" in pre,
        pure=bool(m.group("pure")),
    )


def statements(body: str):
    """Yield the statements of a class or namespace body: 'stmt;', a
    '%macro(...)', 'label:', or a declaration with an inline body '{...}'
    (the body is dropped)."""
    i, n = 0, len(body)
    cur = ""
    while i < n:
        c = body[i]
        if c == "%" and not cur.strip():
            m = re.match(r"%(\w+)\s*", body[i:])
            name = m.group(1)
            j = i + m.end()
            if name == "extend":
                k = body.index("{", j)
                e = match_brace(body, k)
                yield ("extend", body[j:k].strip(), body[k + 1 : e - 1])
                i = e
                if i < n and body[i] == ";":
                    i += 1
                cur = ""
                continue
            if name in (
                "template",
                "feature",
                "rename",
                "ignore",
                "nodefaultctor",
                "include",
                "import",
            ):
                k = body.index(";", i) if ";" in body[i:] else n
                nl = body.find("\n", i)
                if name in ("include", "import") and nl != -1 and nl < k:
                    k = nl
                yield ("directive", body[i:k])
                i = k + 1
                cur = ""
                continue
            if j < n and body[j] == "(":
                k = match_brace(body, j, "(", ")")
                arg = body[j + 1 : k - 1]
                j = k
                rest = re.match(r"\s*", body[j:])
                j += rest.end()
                if j < n and body[j] == "{":
                    k = match_brace(body, j)
                    yield ("macro_block", name, arg, body[j + 1 : k - 1])
                    j = k
                else:
                    yield ("macro", name, arg)
                if j < n and body[j] == ";":
                    j += 1
            elif j < n and body[j] == "{":
                k = match_brace(body, j)
                yield ("macro_block", name, "", body[j + 1 : k - 1])
                j = k
            else:
                # e.g. %feature("director") X; handled as a statement
                k = body.index(";", i)
                yield ("directive", body[i:k])
                j = k + 1
            i = j
            cur = ""
            continue
        if c == ";":
            if cur.strip():
                yield ("stmt", cur.strip())
            cur = ""
        elif c == "{":
            k = match_brace(body, i)
            s = cur.strip()
            if re.match(
                r"^(template\s*<[^>]*>\s*)?(class|struct)\b", s
            ) or s.startswith("namespace"):
                yield ("block", s, body[i + 1 : k - 1])
            elif s:
                # Inline function body; drop the body and any initializer list
                s = re.sub(r"\)\s*:\s*[^()]*(\([^()]*\)\s*,?\s*)*$", ")", s)
                yield ("stmt", s)
            cur = ""
            i = k
            if i < n and body[i] == ";":
                i += 1
            continue
        elif c == ":" and re.match(r"^\s*(public|protected|private)\s*$", cur):
            yield ("label", cur.strip())
            cur = ""
        else:
            cur += c
        i += 1
    if cur.strip():
        yield ("stmt", cur.strip())


# ---------------------------------------------------------------------------
# Converter
# ---------------------------------------------------------------------------


class Converter:
    def __init__(
        self,
        ns: str,
        header_dirs: list[Path],
        director_macro: str,
        prefix: str = "nb",
        allow_none: bool = False,
    ):
        self.prefix = prefix
        # Add .none() to every shared_ptr argument (what SWIG accepted). Off by
        # default: with .none() on a parameter C++ dereferences, python
        # crashes; add it by hand where C++ accepts a null pointer.
        self.allow_none = allow_none
        self.ns = ns
        self.header_dirs = header_dirs
        self.director_macro = director_macro
        self.modules: dict[str, Module] = {}
        self.classes: dict[str, ClassInfo] = {}  # by C++ name (short)
        self.templates: dict[str, ClassInfo] = {}
        self.instances: dict[str, str] = {}  # C++ type -> python name
        self.vectors: list[tuple[str, str, Module]] = []  # (type, pyname, module)
        self.directors: set[str] = set()
        self.header_text: dict[str, str] = {}
        self.extend_pickle_vector: set[str] = set()

    # -- reading -----------------------------------------------------------

    def read(self, path: Path) -> None:
        text = path.read_text()
        mod = Module(path.stem, path)
        self.modules[mod.name] = mod
        for b in re.findall(r"%\{(.*?)%\}", text, flags=re.S):
            mod.includes += re.findall(r'#include\s*"([^"]+)"', b)
        text = re.sub(r"%\{.*?%\}", "", text, flags=re.S)
        text = strip_comments(text)
        for m in re.finditer(
            r'%base_import\((\w+)\)|%import\s*(?:\([^)]*\))?\s*"(\w+)\.i"', text
        ):
            mod.imports.append(m.group(1) or m.group(2))
        self.parse_body(text, mod, None)

    def parse_body(self, body: str, mod: Module, cls: ClassInfo | None) -> None:
        public = True
        for st in statements(body):
            kind = st[0]
            if kind == "label":
                public = st[1] == "public"
                continue
            if kind == "block":
                head, inner = st[1], st[2]
                if head.startswith("namespace"):
                    self.parse_body(inner, mod, cls)
                    continue
                m = re.match(
                    r"^(?:template\s*<\s*class\s+(\w+)\s*>\s*)?(?:class|struct)\s+(\w+)\s*(?::\s*(.*))?$",
                    head,
                    re.S,
                )
                tparam, name, bases = m.group(1), m.group(2), m.group(3)
                bl = [
                    re.sub(r"^(public|protected|private|virtual|\s)+", "", b)
                    for b in split_top(bases or "")
                ]
                ci = ClassInfo(
                    name, mod.name, [norm_type(b) for b in bl], inner, template=tparam
                )
                self.parse_body(inner, mod, ci)
                if tparam:
                    self.templates[name] = ci
                else:
                    self.classes[name] = ci
                    mod.items.append(("class", ci))
                continue
            if kind in ("macro", "macro_block"):
                self.macro(st, mod, cls)
                continue
            if kind == "directive":
                self.directive(st[1], mod, cls)
                continue
            if kind == "extend":
                (cls.hand if cls else mod.hand).append(f"%extend {st[1]} {{{st[2]}}}")
                continue
            stmt = st[1]
            if (
                stmt.startswith("template")
                or stmt.startswith("class ")
                or stmt.startswith("using")
            ):
                continue
            if cls is None:
                if stmt.startswith("namespace") or stmt.startswith("%"):
                    continue
                f = parse_func(stmt)
                if f:
                    mod.functions.append(f)
                    mod.items.append(("func", f))
                else:
                    mod.hand.append(stmt)
                continue
            if "~" in stmt:
                continue
            f = parse_func(stmt)
            if f is None:
                cls.hand.append(stmt)
                continue
            if f.name.startswith("~"):
                continue
            if f.name == cls.name and not f.ret:
                if public:
                    cls.ctors.append(f)
                elif not f.params:
                    cls.protected_default_ctor = True
                continue
            if not public:
                continue
            cls.methods.append(f)

    def macro(self, st, mod: Module, cls: ClassInfo | None) -> None:
        name, arg = st[1], st[2]
        args = split_top(arg)
        if name in (
            "skeleton_shared_ptr",
            "python_export",
            "shared_ptr_value_traits",
            "base_import",
            "include",
            "import",
            "module",
        ):
            return
        if name == "template":
            pyname, typ = arg.strip(), None
            return
        if cls is not None:
            if name.startswith("python_attribute"):
                if name == "python_attribute_std_array":
                    cls.attrs.append(
                        Attr(
                            name, args[0], args[0], f"std::array<{args[1]}, {args[2]}>"
                        )
                    )
                elif name.startswith("python_attribute2"):
                    cls.attrs.append(
                        Attr(name, args[0], args[1], norm_type(", ".join(args[2:])))
                    )
                else:
                    cls.attrs.append(
                        Attr(name, args[0], args[0], norm_type(", ".join(args[1:])))
                    )
                return
            if name == "pickle_serialization":
                cls.pickle.append("cxx_python_rules_nb::pickle_serialization(c);")
                return
            if name == "pickle_serialization_dir":
                cls.pickle.append("cxx_python_rules_nb::pickle_serialization_dir(c);")
                return
            if name == "pickle_init":
                attrs = ", ".join('"' + a.replace("self.", "") + '"' for a in args[1:])
                cls.pickle.append(
                    f"cxx_python_rules_nb::pickle_init(c, {args[0]}, {{{attrs}}});"
                )
                return
            if name == "pythoncode":
                cls.hand.append("%pythoncode " + st[-1].strip())
                return
        if name == self.director_macro:
            self.pending_director_serialization = getattr(
                self, "pending_director_serialization", []
            )
            self.pending_director_serialization.append(args[1])
            return
        if name == "extend" or (st[0] == "macro_block" and name == "extend"):
            pass
        text = f"%{name}({arg})" + (
            " {" + st[3] + "}" if st[0] == "macro_block" else ""
        )
        (cls.hand if cls else mod.hand).append(text)

    def directive(self, d: str, mod: Module, cls: ClassInfo | None) -> None:
        d = d.strip()
        m = re.match(r'%feature\("director"\)\s*(.+)$', d)
        if m:
            self.directors.add(norm_type(m.group(1)).replace(f"{self.ns}::", ""))
            return
        m = re.match(r"%template\((\w+)\)\s*(.+)$", d, re.S)
        if m:
            pyname, typ = m.group(1), norm_type(m.group(2))
            typ = typ.replace(f"{self.ns}::", "")
            if typ.startswith("std::vector"):
                self.vectors.append((typ, pyname, mod))
                mod.items.append(("vector", (typ, pyname)))
            else:
                self.instances[typ] = pyname
                mod.items.append(("instance", (typ, pyname)))
            return
        m = re.match(r"%extend\s+(\S+)", d)
        if re.match(r"%(nodefaultctor)\s+(\S+)", d):
            n = (
                re.match(r"%nodefaultctor\s+(\S+)", d)
                .group(1)
                .replace(f"{self.ns}::", "")
            )
            self.nodefault = getattr(self, "nodefault", set())
            self.nodefault.add(n)
            return
        if re.match(
            r"%(include|import|base_import|module|rename|naturalvar|ignore)\b", d
        ):
            return
        (cls.hand if cls else mod.hand).append(d)

    # -- analysis ----------------------------------------------------------

    def header_for(self, cls: str) -> str:
        if cls in self.header_text:
            return self.header_text[cls]
        txt = ""
        for d in self.header_dirs:
            for h in d.glob("*.h"):
                t = h.read_text()
                if re.search(rf"\bclass\s+{cls}\b[^;]*\{{", t):
                    txt = t
                    break
            if txt:
                break
        self.header_text[cls] = txt
        return txt

    def virtual_bases(self, cls: str) -> set[str]:
        t = self.header_for(cls)
        m = re.search(rf"\bclass\s+{cls}\s*:\s*([^{{]*)\{{", t)
        res = set()
        if m:
            for b in split_top(m.group(1)):
                if "virtual" in b.split():
                    res.add(
                        re.sub(
                            r"^(public|protected|private|virtual|\s)+", "", b
                        ).strip()
                    )
        return res

    def instantiate(self, typ: str, pyname: str) -> ClassInfo:
        m = re.match(r"(\w+)<(.+)>$", typ)
        tname, targ = m.group(1), m.group(2)
        t = self.templates[tname]
        sub = lambda s: re.sub(rf"\b{t.template}\b", targ, s)
        ci = ClassInfo(
            typ,
            t.module,
            [sub(b) for b in t.bases],
            t.body,
            ctors=[self.sub_func(f, sub) for f in t.ctors],
            methods=[self.sub_func(f, sub) for f in t.methods],
            attrs=[Attr(a.kind, a.pyname, a.cppname, sub(a.type)) for a in t.attrs],
            pickle=list(t.pickle),
            hand=list(t.hand),
        )
        ci.ctors = [Func(typ, "", f.params) for f in ci.ctors]
        return ci

    @staticmethod
    def sub_func(f: Func, sub) -> Func:
        return Func(
            f.name,
            sub(f.ret),
            [Param(sub(p.type), p.name, p.default) for p in f.params],
            f.const,
            f.static,
            f.virtual,
            f.pure,
        )

    def finish(self) -> None:
        # Template instances become classes, named by the python name.
        for mod in self.modules.values():
            items = []
            for kind, it in mod.items:
                if kind == "instance":
                    typ, pyname = it
                    ci = self.instantiate(typ, pyname)
                    ci.module = mod.name
                    ci.pyname = pyname
                    self.classes[typ] = ci
                    items.append(("class", ci))
                else:
                    items.append((kind, it))
            mod.items = items
        nodefault = getattr(self, "nodefault", set())
        for name, ci in self.classes.items():
            ci.pyname = ci.pyname or name
            ci.cpp = f"{self.ns}::{name}" if "<" not in name else self.qualify(name)
            ci.director = name in self.directors
            ci.nodefaultctor = name in nodefault
        for n in getattr(self, "pending_director_serialization", []):
            self.classes[n].director_serialization = True
        # Which base becomes the nanobind base, and which classes are reached
        # only as secondary or virtual bases.
        self.secondary: set[str] = set(["GenericObject"])
        for name, ci in self.classes.items():
            ci.nb_base = None
            vb = self.virtual_bases(name.split("<")[0]) if "<" not in name else set()
            for i, b in enumerate(ci.bases):
                usable = b != "GenericObject" and b in self.classes and b not in vb
                if i == 0 and usable:
                    ci.nb_base = b
                elif b != "GenericObject":
                    self.secondary.add(b)
            ci.extra_bases = [
                b for b in ci.bases if b != ci.nb_base and b in self.classes
            ]

    def qualify(self, t: str) -> str:
        """Add the namespace to the class names known to us in type t."""

        def repl(m):
            w = m.group(0)
            if w in self.classes or w in self.templates or w == "GenericObject":
                return f"{self.ns}::{w}"
            return w

        return re.sub(r"(?<![:\w])[A-Z]\w*", repl, t)

    # -- output ------------------------------------------------------------

    def all_methods(self, ci: ClassInfo, seen: set[str] | None = None):
        """Methods and attributes of ci and its bases (for bases not bound
        as the nanobind base, and for director overrides)."""
        res_m, res_a = list(ci.methods), list(ci.attrs)
        for b in ci.bases:
            if b in self.classes:
                m, a = self.all_methods(self.classes[b])
                res_m += m
                res_a += a
        return res_m, res_a

    def inherited_unbound(self, ci: ClassInfo):
        """Methods/attributes of the bases that python won't inherit."""
        res_m, res_a = [], []
        if ci.nb_base:
            bound_m, bound_a = self.all_methods(self.classes[ci.nb_base])
        else:
            bound_m, bound_a = [], []
        bound_names = {f.name for f in bound_m} | {a.pyname for a in bound_a}
        own = {f.name for f in ci.methods} | {a.pyname for a in ci.attrs}
        for b in ci.extra_bases:
            m, a = self.all_methods(self.classes[b])
            for f in m:
                if f.name not in bound_names and f.name not in own and not f.static:
                    res_m.append(f)
                    own.add(f.name)
            for at in a:
                if at.pyname not in bound_names and at.pyname not in own:
                    res_a.append(at)
                    own.add(at.pyname)
        return res_m, res_a

    def ptype(self, t: str) -> str:
        """Lambda parameter type for a .i parameter type."""
        q = self.qualify(t)
        m = re.match(r"^(const )?(\w[\w:<>, ]*?) &$", t)
        if (
            m
            and m.group(2) in self.secondary
            and m.group(2) in self.classes | {"GenericObject": None}
        ):
            return f"const std::shared_ptr<{self.qualify(m.group(2))}>&"
        return q

    def call_args(
        self, f: Func, pre: list[str], post: list[str]
    ) -> tuple[list[str], list[str], list[str]]:
        """(lambda params, call arguments, output names). Statements to run
        before the call are added to pre, and after it to post (the flush of
        an output stream, so an error in its last write is raised)."""
        lp, ca, outs = [], [], []
        for p in f.params:
            t = self.qualify(p.type)
            if p.name.startswith("OUTPUT"):
                base = re.sub(r"\s*&$", "", t)
                o = f"out{len(outs)}"
                pre.append(f"{base} {o}{{}};")
                ca.append(o)
                outs.append(o)
                continue
            if re.match(r"^std::ostream &$", t):
                lp.append(f"nb::object {p.name}")
                pre.append(f"cxx_python_rules_nb::py_ostream {p.name}_os({p.name});")
                post.append(f"{p.name}_os.flush();")
                ca.append(f"{p.name}_os")
                continue
            if re.match(r"^std::istream &$", t):
                lp.append(f"nb::object {p.name}")
                pre.append(f"cxx_python_rules_nb::py_istream {p.name}_is({p.name});")
                ca.append(f"{p.name}_is")
                continue
            pt = self.ptype(p.type)
            name = (
                p.name
                if not p.name.startswith(("SHARED_PTR_NO_OWN", "FORCE_COPY"))
                else f"arg{len(lp)}"
            )
            lp.append(f"{pt} {name}")
            if p.name == "FORCE_COPY":
                ca.append(f"{name}.copy()")
            elif pt.startswith("const std::shared_ptr<") and not t.startswith(
                "const std::shared_ptr"
            ):
                ca.append(f"*{name}")
            else:
                ca.append(name)
        return lp, ca, outs

    def is_simple(self, f: Func) -> bool:
        """True if f can be bound with a member/function pointer: no output,
        stream, FORCE_COPY or SHARED_PTR_NO_OWN arguments, and no argument
        that has to be taken as a shared_ptr (a reference to a secondary or
        virtual base)."""
        for p in f.params:
            t = self.qualify(p.type)
            if p.name.startswith(("OUTPUT", "SHARED_PTR_NO_OWN", "FORCE_COPY")):
                return False
            if re.match(r"^std::(o|i)stream &$", t):
                return False
            if self.ptype(p.type) != t:
                return False
        return True

    def fptr(self, scope: str, f: Func, overloaded: bool, member: bool = True) -> str:
        """&Scope::f, or nb::overload_cast<...>(&Scope::f[, nb::const_]) if
        f is overloaded."""
        ptr = f"&{scope}::{f.name}" if scope else f"&{f.name}"
        if not overloaded:
            return ptr
        types = ", ".join(self.qualify(p.type) for p in f.params)
        c = ", nb::const_" if member and f.const and not f.static else ""
        return f"nb::overload_cast<{types}>({ptr}{c})"

    def unqualify(self, text: str) -> str:
        """Drop "Ns::" outside string literals (the files have "using
        namespace Ns;"); docstring keys keep the full name."""
        parts = text.split('"')
        for i in range(0, len(parts), 2):
            parts[i] = re.sub(rf"\b{self.ns}::", "", parts[i])
        return '"'.join(parts)

    def is_bound_ref(self, t: str) -> bool:
        """Is t a reference to a class bound in this module set?"""
        m = re.match(r"^(const )?(.+?) &$", self.qualify(t))
        if not m:
            return False
        name = m.group(2).replace(f"{self.ns}::", "")
        return name in self.classes or self.qualify(m.group(2)) in self.instances

    def arg_names(self, f: Func) -> str:
        res = []
        for i, p in enumerate(f.params):
            if p.name.startswith("OUTPUT"):
                continue
            n = (
                p.name
                if not p.name.startswith(("SHARED_PTR_NO_OWN", "FORCE_COPY"))
                else f"arg{i}"
            )
            a = f'"{n}"_a'
            # SWIG accepted None (an empty pointer) for a shared_ptr argument;
            # nanobind only does with .none(). See allow_none.
            if self.allow_none and "shared_ptr" in p.type:
                a += ".none()"
            if p.default is not None:
                a += f" = {p.default}"
            res.append(a)
        return "".join(", " + a for a in res)

    def method_def(
        self,
        ci_cpp: str,
        f: Func,
        pyname: str | None = None,
        self_t: str | None = None,
        tramp: str | None = None,
        overloaded: bool = False,
        doc_expr: str | None = None,
    ) -> str:
        pyname = pyname or f.name
        doc = doc_expr or f'doc("{f.name}")'
        d = "def_static" if f.static else "def"
        dispatch = tramp and f.virtual and not f.pure and not f.static
        if self.is_simple(f) and not dispatch:
            return (
                f'    c.{d}("{pyname}", {self.fptr(ci_cpp, f, overloaded)}'
                f"{self.arg_names(f)}, {doc});"
            )
        pre: list[str] = []
        post: list[str] = []
        lp, ca, outs = self.call_args(f, pre, post)
        after = " ".join(post)

        def ret(c: str) -> str:
            """Statements that call c, run post and return the result.
            decltype(auto) keeps a reference result a reference."""
            if not post:
                return f"return {c};"
            if f.ret == "void":
                return f"{c}; {after} return;"
            return f"decltype(auto) r = {c}; {after} return r;"

        self_t = self_t or ci_cpp
        if f.static:
            params = ", ".join(lp)
            call = f"{ci_cpp}::{f.name}({', '.join(ca)})"
        else:
            st = f"const {self_t}&" if f.const else f"{self_t}&"
            params = ", ".join([f"{st} self"] + lp)
            call = f"self.{f.name}({', '.join(ca)})"
            if tramp and f.virtual and not f.pure and not outs:
                # A python subclass calling the base (super().f()) must get
                # the C++ implementation, not the virtual function, which
                # would call the python override again.
                c = "const " if f.const else ""
                qcall = f"self.{ci_cpp}::{f.name}({', '.join(ca)})"
                first = ret(qcall) if not post else f"{{ {ret(qcall)} }}"
                return (
                    f'    c.def("{pyname}", []({params}) -> decltype(auto) {{ {" ".join(pre)} '
                    f"if(dynamic_cast<{c}{tramp}*>(&self)) {first} {ret(call)} }}"
                    f"{self.arg_names(f)}, {doc});"
                )
        if outs:
            if f.ret == "void":
                body = f"{call}; " + (f"{after} " if post else "")
                res = (
                    outs[0] if len(outs) == 1 else f"nb::make_tuple({', '.join(outs)})"
                )
                body += f"return {res};"
            else:
                body = (
                    f"auto r = {call}; "
                    + (f"{after} " if post else "")
                    + f"return nb::make_tuple(r, {', '.join(outs)});"
                )
            lam = f"[]({params}) {{ {' '.join(pre)} {body} }}"
        else:
            lam = f"[]({params}) -> decltype(auto) {{ {' '.join(pre)} {ret(call)} }}"
        return f'    c.{d}("{pyname}", {lam}{self.arg_names(f)}, {doc});'

    def attr_def(
        self, ci_cpp: str, a: Attr, self_t: str | None = None, tramp: str | None = None
    ) -> list[str]:
        self_t = self_t or ci_cpp
        t = self.qualify(a.type.replace("virtual ", "").strip())
        doc = f'doc("{a.cppname}")'
        get_self = f"{self_t}&" if "nonconst" in a.kind else f"const {self_t}&"
        if a.kind == "python_attribute_std_array":
            n, tt = a.cppname, re.match(r"std::array<(\w+), (\d+)>", t)
            return [
                f'    c.def_prop_rw("{a.pyname}", [](const {self_t}& self) {{ return self.{n}; }}, '
                f"[]({self_t}& self, nb::handle v) {{ self.{n} = cxx_python_rules_nb::to_std_array<{tt.group(1)}, {tt.group(2)}>(v); }}, {doc});"
            ]
        # The getter by member pointer (the overload with no arguments; a
        # setter may have the same name). The setter stays a lambda taking
        # const T&, which works whether the C++ takes T or const T&.
        cq = "" if "nonconst" in a.kind else ", nb::const_"
        getter = f"nb::overload_cast<>(&{ci_cpp}::{a.cppname}{cq})"
        setter = f"[]({self_t}& self, const {t}& v) {{ self.{a.cppname}(v); }}"
        if a.kind == "python_attribute_with_set_virtual" and tramp:
            getter = (
                f"[]({get_self} self) -> decltype(auto) {{ if(dynamic_cast<const {tramp}*>(&self)) "
                f"return self.{ci_cpp}::{a.cppname}(); return self.{a.cppname}(); }}"
            )
            setter = (
                f"[]({self_t}& self, const {t}& v) {{ if(dynamic_cast<{tramp}*>(&self)) "
                f"self.{ci_cpp}::{a.cppname}(v); else self.{a.cppname}(v); }}"
            )
        if a.kind == "python_attribute_with_set_virtual":
            return [
                f'    c.def("_v_{a.pyname}", {getter});',
                f'    c.def("_v_{a.pyname}", {setter}, "V"_a);',
                f'    c.def_prop_rw("{a.pyname}", [](nb::handle self) {{ return self.attr("_v_{a.pyname}")(); }}, '
                f'[](nb::handle self, nb::handle v) {{ self.attr("_v_{a.pyname}")(v); }}, {doc});',
            ]
        if "with_set" in a.kind:
            return [f'    c.def_prop_rw("{a.pyname}", {getter}, {setter}, {doc});']
        return [f'    c.def_prop_ro("{a.pyname}", {getter}, {doc});']

    def trampoline(self, ci: ClassInfo) -> list[str]:
        name = "Py" + re.sub(r"\W", "", ci.pyname)
        ms, attrs = self.all_methods(ci)
        out = [
            f"class {name} : public {ci.cpp} {{",
            "public:",
            f"  NB_TRAMPOLINE({ci.cpp});",
        ]
        if ci.director_serialization:
            out.append(f"  {name}() : {ci.cpp}() {{}}")
        seen = set()
        virt = [f for f in ms if f.virtual and not f.static]
        for a in attrs:
            if a.kind in ("python_attribute_with_set_virtual",):
                t = self.qualify(a.type)
                virt.append(
                    Func(
                        a.cppname,
                        t,
                        [],
                        const=True,
                        virtual=True,
                        pyname=f"_v_{a.pyname}",
                    )
                )
                pt = t if t in SCALARS else f"const {t} &"
                # The setter's parameter type as the C++ header has it.
                hm = re.search(
                    rf"virtual\s+void\s+{a.cppname}\s*\(\s*([^)]*?)\s*\w+\s*\)",
                    self.header_for(ci.name.split("<")[0]),
                )
                if hm:
                    pt = self.qualify(norm_type(hm.group(1)))
                virt.append(
                    Func(
                        a.cppname,
                        "void",
                        [Param(pt, "V")],
                        virtual=True,
                        pyname=f"_v_{a.pyname}",
                    )
                )
            elif "virtual" in a.type or a.kind == "python_attribute_abstract":
                t = self.qualify(a.type.replace("virtual ", "").strip())
                virt.append(
                    Func(
                        a.cppname,
                        t,
                        [],
                        const=True,
                        virtual=True,
                        pyname=f"_v_{a.pyname}",
                        pure=a.kind == "python_attribute_abstract",
                    )
                )
        for f in virt:
            key = (f.name, tuple(self.qualify(p.type) for p in f.params), f.const)
            if key in seen:
                continue
            seen.add(key)
            ps = ", ".join(f"{self.qualify(p.type)} {p.name}" for p in f.params)
            an = ", ".join(p.name for p in f.params)
            macro = "NB_OVERRIDE_PURE_NAME" if f.pure else "NB_OVERRIDE_NAME"
            pyn = f.pyname or f.name
            refs = [p for p in f.params if self.is_bound_ref(p.type)]
            if refs and len(f.params) == 1 and not f.pure:
                # NB_OVERRIDE_NAME would pass a copy of the object to python.
                macro = "CXX_PYTHON_RULES_NB_OVERRIDE_REF"
            elif refs:
                out.append(
                    "  // HAND CONVERT: NB_OVERRIDE passes a copy of "
                    + ", ".join(p.name for p in refs)
                    + " to python; see CXX_PYTHON_RULES_NB_OVERRIDE_REF"
                )
            out.append(
                f"  {self.qualify(f.ret)} {f.name}({ps}){' const' if f.const else ''} override"
                f' {{ {macro}("{pyn}", {f.name}{", " + an if an else ""}); }}'
            )
        out.append("};")
        return out

    def class_def(self, ci: ClassInfo) -> list[str]:
        out = []
        tramp = None
        if ci.director:
            out += self.trampoline(ci)
            tramp = "Py" + re.sub(r"\W", "", ci.pyname)
        targs = [ci.cpp]
        if ci.nb_base:
            targs.append(self.classes[ci.nb_base].cpp)
        if tramp:
            targs.append(tramp)
        body = [
            f"  {{",
            f'    const auto doc = cxx_python_rules_nb::docs("{ci.cpp}");',
            f'    nb::class_<{", ".join(targs)}> c(m, "{ci.pyname}", doc());',
        ]
        unused = [b for b in ci.bases if b not in (ci.nb_base, "GenericObject")]
        if unused:
            body.insert(
                1,
                f"    // Not nanobind bases (nanobind allows one, never a virtual "
                f"one): {', '.join(unused)}; their methods are bound here.",
            )
        abstract = ci.nodefaultctor or (
            any(f.pure for f in ci.methods) and not ci.director
        )
        if not abstract:
            for f in ci.ctors:
                types = ", ".join(self.qualify(p.type) for p in f.params)
                if ci.director:
                    body.append(f"    c.def(nb::init<{types}>(){self.arg_names(f)});")
                else:
                    nm = [
                        p.name
                        if not p.name.startswith(("SHARED_PTR_NO_OWN", "FORCE_COPY"))
                        else f"arg{i}"
                        for i, p in enumerate(f.params)
                    ]
                    lp = ", ".join(
                        f"{self.qualify(p.type)} {n}" for p, n in zip(f.params, nm)
                    )
                    an = ", ".join(nm)
                    body.append(
                        f"    c.def(nb::new_([]({lp}) {{ return std::make_shared<{ci.cpp}>({an}); }}){self.arg_names(f)});"
                    )
        extra_m, extra_a = self.inherited_unbound(ci)
        has_print = False
        names = [f.name for f in ci.methods + extra_m]
        for f in ci.methods + extra_m:
            if f.name == "print_to_string":
                has_print = True
                continue
            body.append(
                self.method_def(
                    ci.cpp,
                    f,
                    tramp=tramp if f in ci.methods else None,
                    overloaded=names.count(f.name) > 1,
                )
            )
        for a in ci.attrs + extra_a:
            body += self.attr_def(ci.cpp, a, tramp=tramp if a in ci.attrs else None)
        if has_print or any(
            f.name == "print_to_string" for f in self.all_methods(ci)[0]
        ):
            body.append("    cxx_python_rules_nb::printable(c);")
        for p in ci.pickle:
            body.append("    " + p)
        if ci.pyname in self.extend_pickle_vector:
            body.append("    cxx_python_rules_nb::pickle_vector(c);")
        for h in ci.hand:
            body.append("    // HAND CONVERT: " + h.replace("\n", "\n    //   "))
        body.append(f"    cxx_python_rules_nb::register_type<{ci.cpp}>();")
        if tramp:
            body.append(
                f"    cxx_python_rules_nb::register_alias<{tramp}, {ci.cpp}>();"
            )
        body.append("  }")
        return out, body

    def write(self, out_dir: Path) -> None:
        out_dir.mkdir(parents=True, exist_ok=True)
        opaque = [
            "// The std::vector types bound with nb::bind_vector, declared opaque (a",
            "// python object that refers to the C++ vector, rather than a converted",
            "// list). Every binding file includes this, so all agree.",
            "#pragma once",
            '#include "cxx_python_rules_nb.h"',
        ]
        for typ, _, mod in self.vectors:
            for inc in mod.includes:
                opaque.append(f'#include "{inc}"')
        opaque = list(dict.fromkeys(opaque))
        for typ, _, _ in self.vectors:
            opaque.append(f"NB_MAKE_OPAQUE({self.qualify(typ)})")
        (out_dir / f"{self.prefix}_opaque.h").write_text("\n".join(opaque) + "\n")
        for mod in self.modules.values():
            self.write_module(mod, out_dir)
        order: list[str] = []

        def visit(n: str) -> None:
            if n in order or n not in self.modules:
                return
            for d in self.modules[n].imports:
                visit(d)
            order.append(n)

        for n in self.modules:
            visit(n)
        lines = [
            "// Calls the init function of each binding file. A class has to be",
            "// bound before a class that has it as its nanobind base, so they are",
            f"// called in %import order. Call {self.prefix}_init_all(m) from NB_MODULE.",
            f'#include "{self.prefix}_opaque.h"',
            "",
            "namespace nb = nanobind;",
            "",
        ]
        lines += [f"void init_{n}(nb::module_& m);" for n in order]
        lines += ["", f"void {self.prefix}_init_all(nb::module_& m)", "{"]
        lines += [f"  init_{n}(m);" for n in order]
        lines.append("}")
        (out_dir / f"{self.prefix}_init.cc").write_text("\n".join(lines) + "\n")

    def write_module(self, mod: Module, out_dir: Path) -> None:
        pre, body = [], []
        for kind, it in mod.items:
            if kind == "class":
                o, b = self.class_def(it)
                pre += o
                body += b
            elif kind == "vector":
                typ, pyname = it
                if pyname in self.extend_pickle_vector:
                    body.append(
                        f'  {{\n    auto c = nb::bind_vector<{self.qualify(typ)}>(m, "{pyname}");'
                        "\n    cxx_python_rules_nb::pickle_vector(c);\n  }"
                    )
                else:
                    body.append(
                        f'  nb::bind_vector<{self.qualify(typ)}>(m, "{pyname}");'
                    )
            elif kind == "func":
                f = it
                fnames = [g.name for k, g in mod.items if k == "func"]
                body.append(
                    self.method_def(
                        self.ns,
                        Func(f.name, f.ret, f.params, static=True),
                        overloaded=fnames.count(f.name) > 1,
                        doc_expr=f'cxx_python_rules_nb::doc("{self.ns}::{f.name}")',
                    ).replace("c.def_static", "m.def")
                )
        ds = []
        for kind, it in mod.items:
            if kind == "class" and it.director_serialization:
                t = "Py" + re.sub(r"\W", "", it.pyname)
                ds.append(
                    f"CXX_PYTHON_RULES_NB_DIRECTOR_SERIALIZATION({t}, {it.cpp}, {it.name})"
                )
        hand = "".join(
            "// HAND CONVERT: " + h.replace("\n", "\n//   ") + "\n" for h in mod.hand
        )
        incs = "".join(f'#include "{i}"\n' for i in mod.includes)
        what = ", ".join(mod.includes) or mod.path.name
        anon = ("namespace {\n" + "\n".join(pre) + "\n}\n\n") if pre else ""
        text = self.unqualify(
            f"// Python bindings for {what} (converted from {mod.path.name} by\n"
            f"// cxx_python_rules/nanobind/i_to_nanobind.py).\n"
            f'#include "{self.prefix}_opaque.h"\n{incs}\n'
            f"namespace nb = nanobind;\n"
            f"using namespace nb::literals;\n"
            f"using namespace {self.ns};\n\n"
            + anon
            + "\n".join(ds)
            + ("\n\n" if ds else "")
            + hand
            + f"void init_{mod.name}(nb::module_& m)\n{{\n"
            + "\n".join(body)
            + "\n}\n"
        )
        (out_dir / f"{mod.name}_nb.cc").write_text(text)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("files", nargs="+", type=Path)
    ap.add_argument("--namespace", required=True)
    ap.add_argument("--header-dir", action="append", type=Path, default=[])
    ap.add_argument("--out-dir", type=Path, required=True)
    ap.add_argument(
        "--director-serialization-macro", default="skeleton_director_serialization"
    )
    ap.add_argument(
        "--prefix",
        default="nb",
        help="name of the <prefix>_opaque.h and <prefix>_init.cc files",
    )
    ap.add_argument(
        "--allow-none",
        action="store_true",
        help="add .none() to every shared_ptr argument (what SWIG accepted); "
        "by default add it by hand where C++ accepts a null pointer",
    )
    args = ap.parse_args()
    c = Converter(
        args.namespace,
        args.header_dir,
        args.director_serialization_macro,
        args.prefix,
        args.allow_none,
    )
    for f in args.files:
        c.read(f)
    # %extend std::vector<T> { %pickle_vector(); }
    for f in args.files:
        for m in re.finditer(
            r"%extend\s+(std::vector<[^{]+>)\s*\{([^}]*)\}", f.read_text()
        ):
            if "pickle_vector" in m.group(2):
                typ = norm_type(m.group(1)).replace(f"{args.namespace}::", "")
                for t, pyname, _ in c.vectors:
                    if t == typ:
                        c.extend_pickle_vector.add(pyname)
    c.finish()
    c.write(args.out_dir)
    return 0


if __name__ == "__main__":
    sys.exit(main())
