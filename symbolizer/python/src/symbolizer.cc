#include "symbolizer.hpp"

#include <nanobind/nanobind.h>
// #include <nanobind/stl/array.h>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unordered_map.h>


namespace nb = nanobind;
using namespace nb::literals;

NB_MODULE(_symbolizer, m)
{
    m.doc() = "Symbolizer module";

    nb::class_<Symbolizer::__Version>(m, "version")
        .def_ro_static("major", &Symbolizer::__Version::Major)
        .def_ro_static("minor", &Symbolizer::__Version::Minor)
        .def_ro_static("patch", &Symbolizer::__Version::Patch)
        .def_ro_static("release", &Symbolizer::__Version::Release);

    nb::class_<Symbolizer::__Target>(m, "target")
        .def_ro_static("architecture", &Symbolizer::__Target::Architecture)
        .def_ro_static("system", &Symbolizer::__Target::System);

    nb::enum_<Symbolizer::TraceStyle_t>(m, "TraceStyle_t")
        .value("Modoff", Symbolizer::TraceStyle_t::Modoff)
        .value("FullSymbol", Symbolizer::TraceStyle_t::FullSymbol)
        .export_values();

    nb::class_<Symbolizer::DbgEng_t>(m, "DbgEng_t")
        .def(nb::init<>())
        .def("Init", &Symbolizer::DbgEng_t::Init, nb::arg("DumpPath"), nb::arg("SymbolPath").none())
        .def(
            "Symbolize",
            [](Symbolizer::DbgEng_t& DbgEng,
               const uint64_t SymbolAddress,
               const Symbolizer::TraceStyle_t Style) -> std::optional<std::string>
            {
                auto res = DbgEng.Symbolize(SymbolAddress, Style);
                if ( !res )
                {
                    return {};
                }
                return std::string(res.value());
            })
        .def("Resolve", &Symbolizer::DbgEng_t::Resolve, "SymbolName"_a);
}
