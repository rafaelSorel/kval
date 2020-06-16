
#include "KvalAppShared.h"
#include "KvalAppDeployment.h"
#include "builtin/KvalAppUiPopUp.h"
#include "KvalAppEmbedModules.h"

namespace py = pybind11;
PYBIND11_MAKE_OPAQUE(std::vector<KvalApplication::UserInterface::ModalBaseDialog::Entry>);

namespace KvalApplication {

using namespace UserInterface;

PYBIND11_EMBEDDED_MODULE(intkval, m) {
    py::class_<KApplication>(m, "Application")
        .def(py::init<const string &, const string &>())
        .def("get_id", &KApplication::get_id)
        .def("add_dep", &KApplication::addDep)
        .def("set_launcher", &KApplication::setLauncher)
        .def("__repr__",
            [](const KApplication &a) {
                return "<cppkval.Application app id '" + a.get_id() + "'>";})
            ;
    py::class_<KApplication::Dependency>(m, "Dependency")
            .def(py::init<const string&,const string&,const string&>())
            .def_readwrite("id", &KApplication::Dependency::id)
            .def_readwrite("version", &KApplication::Dependency::version)
            .def_readwrite("uri", &KApplication::Dependency::uri)
            ;
    py::class_<KApplication::Launcher>(m, "Launcher")
            .def(py::init<const string&,const string&,const string&,const string&>())
            .def_readwrite("main", &KApplication::Launcher::main)
            .def_readwrite("daemon", &KApplication::Launcher::daemon)
            .def_readwrite("update", &KApplication::Launcher::update)
            .def_readwrite("package", &KApplication::Launcher::package)
            ;
}

PYBIND11_EMBEDDED_MODULE(kvalui, m) {

    py::register_exception<KvalUiException>(m, "KvalUiException");

    py::enum_<AlertType>(m, "AlertType", py::arithmetic())
        .value("Invalid", AlertType::Invalid)
        .value("Info", AlertType::Info)
        .value("Success", AlertType::Success)
        .value("Warning", AlertType::Warning)
        .value("Error", AlertType::Error)
        .value("Count", AlertType::Count);

    py::class_<Alert, std::shared_ptr<Alert>>(m, "Alert")
        .def(py::init<AlertType,
                      const string&, const string&,
                      const string&, int>(),
             py::arg("type")=AlertType::Invalid,
             py::arg("header")="", py::arg("body")="",
             py::arg("icon")="", py::arg("timeout")=15)
        .def(py::init<>())
        .def_property("type", &Alert::getType, &Alert::setType)
        .def_property("header", &Alert::getHeader, &Alert::setHeader)
        .def_property("body", &Alert::getBody, &Alert::setBody)
        .def_property("icon", &Alert::getIcon, &Alert::setIcon)
        .def_property("timeout", &Alert::getTimeout, &Alert::setTimeout)
        .def("display", &Alert::display)
        .def("clear", &Alert::clear)
        .def("__repr__",
            [](const Alert &a) {
                return "<kvalui.Alert '" + a.getHeader() + "'>";});

    py::enum_<BoxType>(m, "BoxType", py::arithmetic())
        .value("Invalid", BoxType::Invalid)
        .value("Singlebutton", BoxType::Singlebutton)
        .value("Multibutton", BoxType::Multibutton);

    py::enum_<InputType>(m, "InputType", py::arithmetic())
        .value("Invalid", InputType::Invalid)
        .value("Inputlist", InputType::Inputlist)
        .value("Inputkeyboard", InputType::Inputkeyboard)
        .value("Inputfilebrowsing", InputType::Inputfilebrowsing);

    py::class_<ModalBaseDialog::Entry>(m, "Entry")
        .def(py::init<>())
        .def(py::init<int, const string&, const string&>(),
             py::arg("id")=0, py::arg("value")="",py::arg("icon")="")
        .def_readwrite("id", &SimpleDialog::Button::id)
        .def_readwrite("value", &SimpleDialog::Button::value)
        .def_readwrite("icon", &SimpleDialog::Button::icon);

    py::bind_vector<std::vector<ModalBaseDialog::Entry>>(m, "EntryList");

    py::class_<SimpleDialog::Button>(m, "Button")
        .def(py::init<>())
        .def(py::init<int, const string&, const string&, const string&>(),
             py::arg("id")=0, py::arg("value")="",py::arg("icon")="", py::arg("rgb")="")
        .def_readwrite("id", &SimpleDialog::Button::id)
        .def_readwrite("value", &SimpleDialog::Button::value)
        .def_readwrite("icon", &SimpleDialog::Button::icon)
        .def_readwrite("rgb", &SimpleDialog::Button::rgb);

    py::class_<SimpleDialog, std::shared_ptr<SimpleDialog>>(m, "SimpleDialog")
        .def(py::init<BoxType,
                      const string&, const string&,
                      const string&, const string&>(),
             py::arg("type")=BoxType::Invalid,
             py::arg("title")="", py::arg("header")="",
             py::arg("body")="", py::arg("icon")="")
        .def(py::init<>())
        .def_property("type", &SimpleDialog::getType, &SimpleDialog::setType)
        .def_property("title", &SimpleDialog::getTitle, &SimpleDialog::setTitle)
        .def_property("header", &SimpleDialog::getHeader, &SimpleDialog::setHeader)
        .def_property("body", &SimpleDialog::getBody, &SimpleDialog::setBody)
        .def_property("icon", &SimpleDialog::getIcon, &SimpleDialog::setIcon)
        .def("add_button", &SimpleDialog::addButton)
        .def("set_buttons", &SimpleDialog::setButtons)
        .def("get_buttons", &SimpleDialog::getButtons)
        .def("display", &SimpleDialog::display)
        .def("__repr__",
            [](const SimpleDialog &a) {
                return "<kvalui.SimpleDialog '" + a.getTitle() + "'>";});

    py::class_<ProgressDialog, std::shared_ptr<ProgressDialog>>(m, "ProgressDialog")
        .def(py::init<BoxType,
                      const string&, const string&,
                      const string&, const string&>(),
             py::arg("type")=BoxType::Invalid,
             py::arg("title")="", py::arg("header")="",
             py::arg("body")="", py::arg("icon")="")
        .def(py::init<>())
        .def_property("type", &ProgressDialog::getType, &ProgressDialog::setType)
        .def_property("title", &ProgressDialog::getTitle, &ProgressDialog::setTitle)
        .def_property("header", &ProgressDialog::getHeader, &ProgressDialog::setHeader)
        .def_property("body", &ProgressDialog::getBody, &ProgressDialog::setBody)
        .def_property("icon", &ProgressDialog::getIcon, &ProgressDialog::setIcon)
        .def("add_button", &ProgressDialog::addButton)
        .def("set_buttons", &ProgressDialog::setButtons)
        .def("get_buttons", &ProgressDialog::getButtons)
        .def("display", &ProgressDialog::display)
        .def("update", &ProgressDialog::update)
        .def("close", &ProgressDialog::close)
        .def("is_aborted", &ProgressDialog::isAborted)
        .def("__repr__",
            [](const ProgressDialog &a) {
                return "<kvalui.ProgressDialog '" + a.getTitle() + "'>";});

    py::class_<InputDialog, std::shared_ptr<InputDialog>>(m, "InputDialog")
        .def(py::init<InputType,
                      const string&, const string&,
                      const string&, const string&>(),
             py::arg("type")=InputType::Invalid,
             py::arg("title")="", py::arg("header")="",
             py::arg("body")="", py::arg("icon")="")
        .def(py::init<>())
        .def_property("type", &InputDialog::getType, &InputDialog::setType)
        .def_property("title", &InputDialog::getTitle, &InputDialog::setTitle)
        .def_property("header", &InputDialog::getHeader, &InputDialog::setHeader)
        .def_property("body", &InputDialog::getBody, &InputDialog::setBody)
        .def_property("icon", &InputDialog::getIcon, &InputDialog::setIcon)
        .def("input_list", &InputDialog::inputList)
        .def("input_filebrowser", &InputDialog::inputFileBrowser,
             py::arg("path")="", py::arg("filter")="")
        .def("input_keyboard", &InputDialog::inputKeyBoard, py::arg("default")="")
        .def("__repr__",
            [](const InputDialog &a) {
                return "<kvalui.InputDialog '" + a.getTitle() + "'>";});
}

EmbedModules::EmbedModules()
{

}


} // namespace KvalApplication
