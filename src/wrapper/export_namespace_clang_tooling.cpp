#include <boost/python.hpp>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Type.h>
#include <clang/AST/PrettyPrinter.h>
#include <llvm/Support/raw_ostream.h>
#include <clang/AST/Mangle.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Mangler.h>
#include <clang/Basic/TargetInfo.h>
#include <iostream>

namespace autowig
{
    clang::ASTUnit* build_ast_from_code_with_args(const std::string& _code, boost::python::object _args)
    {
        std::vector< std::string > args(boost::python::len(_args));
        for(unsigned int i = 0; i < args.size(); ++i)
        { args[i] = boost::python::extract< std::string >(_args[i]); }
        llvm::Twine code(_code);
        clang::ASTUnit* tu = clang::tooling::buildASTFromCodeWithArgs(code, args).release();
        clang::LangOptions lang;
        lang.CPlusPlus = true;
        clang::PrintingPolicy policy(lang);
        policy.SuppressSpecifiers = false;
        policy.SuppressScope = false;
        policy.SuppressUnwrittenScope = true; 
        tu->getASTContext().setPrintingPolicy(policy);
        return tu;
    }

    std::string get_comment(clang::Decl* decl)
    {
        clang::ASTContext & ast = decl->getASTContext();
        clang::SourceManager &  sm = ast.getSourceManager();
        std::string comment = "";
        clang::RawComment* rawcomment = ast.getRawCommentForDeclNoCache(decl);
        if(rawcomment)
        { comment = rawcomment->getRawText(sm).str(); }
        return comment;
    }

    void unset_type_as_written(clang::ClassTemplateSpecializationDecl* decl)
    { /*decl->setTypeAsWritten(nullptr);*/ }

    unsigned int ast_get_nb_children(clang::ASTUnit& ast)
    {
        unsigned int nb = 0;
        for(auto it = ast.top_level_begin(); it != ast.top_level_end(); ++it)
        { ++nb; }
        return nb;
    }

    unsigned int tcls_get_nb_children(const clang::ClassTemplateDecl& cls)
    {
        unsigned int nb = 0;
        for(auto it = cls.spec_begin(); it != cls.spec_end(); ++it)
        { ++nb; }
        return nb;
    }

    unsigned int decl_get_nb_children(clang::DeclContext& decl)
    {
        unsigned int nb = 0;
        for(auto it = decl.decls_begin(); it != decl.decls_end(); ++it)
        { ++nb; }
        return nb;
    }

    clang::Decl* ast_get_child(clang::ASTUnit& ast, const unsigned int& child)
    {
        auto it = ast.top_level_begin();
        for(unsigned int i = 0; i < child; ++i)
        { ++it; }
        return *it;
    }

    clang::Decl* decl_get_child(const clang::DeclContext& decl, const unsigned int& child)
    {
        auto it = decl.decls_begin();
        for(unsigned int i = 0; i < child; ++i)
        { ++it; }
        return *it;
    }

    clang::Decl* tcls_get_child(const clang::ClassTemplateDecl& cls, const unsigned int& child)
    {
        auto it = cls.spec_begin();
        for(unsigned int i = 0; i < child; ++i)
        { ++it; }
        return *it;
    }

    std::string (clang::QualType::*get_as_string)() const = &clang::QualType::getAsString;

    std::string (clang::NamedDecl::*get_name_as_string)() const = &clang::NamedDecl::getNameAsString;

    std::string named_decl_get_qualified_name(clang::NamedDecl* decl)
    { return decl->getQualifiedNameAsString(); }

    void unset_as_written(std::map< clang::ClassTemplateSpecializationDecl*, clang::TypeSourceInfo* >& mapping, clang::ClassTemplateSpecializationDecl* spec)
    {
        mapping[spec] = spec->getTypeAsWritten();
        spec->setTypeAsWritten(nullptr);
        const clang::TemplateArgumentList &args = spec->getTemplateArgs();        
        for(unsigned int i = 0; i < args.size(); ++i)
        {
            const clang::TemplateArgument & arg = args[i];
            if(arg.getKind() == clang::TemplateArgument::ArgKind::Type)
            {
                clang::QualType qtype = arg.getAsType();
                const clang::Type* ttype = qtype.getTypePtrOrNull();
                if(ttype && ttype->isClassType())
                {
                    clang::ClassTemplateSpecializationDecl* nspec = dynamic_cast< clang::ClassTemplateSpecializationDecl* >(ttype->getAsTagDecl());
                    if(nspec)
                    { unset_as_written(mapping, nspec); }
                }
            }
        }
    }

    void set_as_written(std::map< clang::ClassTemplateSpecializationDecl*, clang::TypeSourceInfo* >& mapping, clang::ClassTemplateSpecializationDecl* spec)
    {
        spec->setTypeAsWritten(mapping[spec]);
        const clang::TemplateArgumentList &args = spec->getTemplateArgs();        
        for(unsigned int i = 0; i < args.size(); ++i)
        {
            const clang::TemplateArgument & arg = args[i];
            if(arg.getKind() == clang::TemplateArgument::ArgKind::Type)
            {
                clang::QualType qtype = arg.getAsType();
                const clang::Type* ttype = qtype.getTypePtrOrNull();
                if(ttype && ttype->isClassType())
                {
                    clang::ClassTemplateSpecializationDecl* nspec = dynamic_cast< clang::ClassTemplateSpecializationDecl* >(ttype->getAsTagDecl());
                    if(nspec)
                    { set_as_written(mapping, nspec); }
                }
            }
        }
    }

    std::string decl_spelling(const clang::NamedDecl& decl)
    {
        std::string spelling = "";
        llvm::raw_string_ostream os(spelling);
        clang::LangOptions lang;
        lang.CPlusPlus = true;
        clang::PrintingPolicy policy(lang);
        policy.SuppressSpecifiers = false;
        policy.SuppressScope = false;
        policy.SuppressUnwrittenScope = true;        
        decl.getNameForDiagnostic(os, policy, true);
        return os.str();
    }

    std::string ta_spelling(const clang::TemplateArgument& ta)
    {
        std::string spelling = "";
        llvm::raw_string_ostream os(spelling);
        clang::LangOptions lang;
        lang.CPlusPlus = true;
        clang::PrintingPolicy policy(lang);
        policy.SuppressSpecifiers = false;
        policy.SuppressScope = false;
        policy.SuppressUnwrittenScope = true;        
        ta.print(policy, os);
        return os.str();
    }

    std::string spec_get_name_as_string(clang::ClassTemplateSpecializationDecl* spec)
    {
        std::map< clang::ClassTemplateSpecializationDecl*, clang::TypeSourceInfo* > mapping;
        unset_as_written(mapping, spec);
        //clang::TypeSourceInfo* tsi = spec->getTypeAsWritten();
        //spec->setTypeAsWritten(nullptr);
        std::string spelling = "";
        llvm::raw_string_ostream os(spelling);
        os << spec->getName();
        const clang::TemplateArgumentList &args = spec->getTemplateArgs();
        clang::LangOptions lang;
        lang.CPlusPlus = true;
        clang::PrintingPolicy policy(lang);
        policy.SuppressSpecifiers = false;
        policy.SuppressScope = false;
        policy.SuppressUnwrittenScope = true;
        clang::TemplateSpecializationType::PrintTemplateArgumentList(os, args.data(),
                                                                  args.size(),
                                                                  policy);
        std::string res = os.str();
        set_as_written(mapping, spec);        
        //spec->setTypeAsWritten(tsi);        
        return res;
    }

    clang::NamespaceDecl * decl_cast_as_namespace(clang::DeclContext * decl)
    { return static_cast< clang::NamespaceDecl * >(decl); }

    clang::RecordDecl * decl_cast_as_record(clang::DeclContext * decl)
    { return static_cast< clang::RecordDecl * >(decl); }

    unsigned int cxxrecord_get_nb_constructors(const clang::CXXRecordDecl& decl)
    {
        unsigned int nb = 0;
        for(auto it = decl.ctor_begin(); it != decl.ctor_end(); ++it)
        { ++nb; }
        return nb;
    }

    clang::Decl* cxxrecord_get_constructor(const clang::CXXRecordDecl& decl, const unsigned int& index)
    {
        auto it = decl.ctor_begin();
        for(unsigned int i = 0; i < index; ++i)
        { ++it; }
        return *it;
    }

    bool cxxrecord_is_copyable(const clang::CXXRecordDecl& decl)
    {
        auto it = decl.ctor_begin();
        clang::CXXDestructorDecl* dtor = decl.getDestructor();
        bool res = !decl.hasUninitializedReferenceMember() && dtor && dtor->getAccess() == clang::AccessSpecifier::AS_public && !dtor->isDeleted() && it != decl.ctor_end();
        if(res)
        {
            while(res && it != decl.ctor_end())
            {
                res = !(*it)->isCopyConstructor();
                if(res)
                { ++it; }
            }
            if(!res)
            { res = (*it)->isDeleted() || (*it)->getAccess() != clang::AccessSpecifier::AS_public; }
            else
            { res = !res; }
        }
        return !res;
    }

    clang::CXXBaseSpecifier const * cxxrecord_get_base(const clang::CXXRecordDecl& decl, const unsigned int& base)
    {
        auto it = decl.bases_begin();
        for(unsigned int i = 0; i < base; ++i)
        { ++it; }
        return it;
    }

    clang::CXXBaseSpecifier const * cxxrecord_get_virtual_base(const clang::CXXRecordDecl& decl, const unsigned int& base)
    {
        auto it = decl.vbases_begin();
        for(unsigned int i = 0; i < base; ++i)
        { ++it; }
        return it;
    }

    /*std::string decl_get_filename(clang::Decl* decl)
    {
        clang::ASTContext & ast = decl->getASTContext();
        clang::SourceManager &  sm = ast.getSourceManager();
        return sm.getFilename(decl->getLocation()).str();
    }*/

    std::string string_ref_str(llvm::StringRef* sr)
    { return sr->str(); }

    std::string func_get_mangling(clang::FunctionDecl* decl)
    {

        clang::ASTContext & ast = decl->getASTContext();
        clang::MangleContext * mc = ast.createMangleContext();
        std::string frontend;
        llvm::raw_string_ostream frontendos(frontend);
        if(dynamic_cast< clang::CXXConstructorDecl * >(decl))
        { mc->mangleCXXCtor(dynamic_cast< clang::CXXConstructorDecl * >(decl), clang::CXXCtorType::Ctor_Complete, frontendos); }
        else if(dynamic_cast< clang::CXXDestructorDecl * >(decl))
        { mc->mangleCXXDtor(dynamic_cast< clang::CXXDestructorDecl * >(decl), clang::CXXDtorType::Dtor_Complete, frontendos); }
        else
        { mc->mangleName(decl, frontendos); }
        /*llvm::DataLayout* data_layout = new llvm::DataLayout(ast.getTargetInfo().getTargetDescription());
        llvm::Mangler middleend(data_layout);

        std::string backend;
        llvm::raw_string_ostream backendos(backend);
        middleend.getNameWithPrefix(backendos, llvm::Twine(frontendos.str()));
        delete data_layout;
        return backendos.str();*/
	delete mc;
	return frontendos.str();
    }
}

void export_namespace_clang_tooling()
{
    std::string clang_name = boost::python::extract< std::string >(boost::python::scope().attr("__name__") + ".clang");
    boost::python::object clang_module(boost::python::handle<  >(boost::python::borrowed(PyImport_AddModule(clang_name.c_str()))));
    boost::python::scope().attr("clang") = clang_module;
    boost::python::scope clang_scope = clang_module;
    std::string tooling_name = boost::python::extract< std::string >(boost::python::scope().attr("__name__") + ".tooling");
    boost::python::object tooling_module(boost::python::handle<  >(boost::python::borrowed(PyImport_AddModule(tooling_name.c_str()))));
    boost::python::scope().attr("tooling") = tooling_module;
    boost::python::scope tooling_scope = tooling_module;
    boost::python::def("build_ast_from_code_with_args", ::autowig::build_ast_from_code_with_args, boost::python::return_value_policy< boost::python::manage_new_object >());
    boost::python::def("get_comment", &::autowig::get_comment);
    boost::python::def("get_name", &::clang::NamedDecl::getNameAsString);
    boost::python::def("ast_get_nb_children", ::autowig::ast_get_nb_children);
    boost::python::def("decl_get_nb_children", ::autowig::decl_get_nb_children);
    boost::python::def("tcls_get_nb_children", ::autowig::tcls_get_nb_children);
    boost::python::def("ast_get_child", ::autowig::ast_get_child, boost::python::return_value_policy< boost::python::reference_existing_object >());
    boost::python::def("decl_get_child", ::autowig::decl_get_child, boost::python::return_value_policy< boost::python::reference_existing_object >());
    boost::python::def("tcls_get_child", ::autowig::tcls_get_child, boost::python::return_value_policy< boost::python::reference_existing_object >());
    boost::python::def("get_as_string", ::autowig::get_as_string);
    boost::python::def("get_name", ::autowig::get_name_as_string);
    boost::python::def("spec_get_name", ::autowig::spec_get_name_as_string);
    boost::python::def("decl_cast_as_namespace", ::autowig::decl_cast_as_namespace, boost::python::return_value_policy< boost::python::reference_existing_object >());
    boost::python::def("decl_cast_as_record", ::autowig::decl_cast_as_record, boost::python::return_value_policy< boost::python::reference_existing_object >());
    boost::python::def("decl_spelling", ::autowig::decl_spelling);
    boost::python::def("ta_spelling", ::autowig::ta_spelling);
    boost::python::def("cxxrecord_get_nb_constructors", ::autowig::cxxrecord_get_nb_constructors);
    boost::python::def("cxxrecord_get_constructor", ::autowig::cxxrecord_get_constructor, boost::python::return_value_policy< boost::python::reference_existing_object >());
    boost::python::def("cxxrecord_is_copyable", ::autowig::cxxrecord_is_copyable);
    boost::python::def("cxxrecord_is_copyable", ::autowig::cxxrecord_is_copyable);
    boost::python::def("cxxrecord_get_base", ::autowig::cxxrecord_get_base, boost::python::return_value_policy< boost::python::reference_existing_object >());
    boost::python::def("cxxrecord_get_virtual_base", ::autowig::cxxrecord_get_virtual_base, boost::python::return_value_policy< boost::python::reference_existing_object >());
    boost::python::def("string_ref_str", ::autowig::string_ref_str);
    boost::python::def("func_get_mangling", ::autowig::func_get_mangling);
    boost::python::def("named_decl_get_qualified_name", ::autowig::named_decl_get_qualified_name);
    boost::python::def("unset_type_as_written", ::autowig::unset_type_as_written);
}
