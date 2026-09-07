#pragma once

#include <UI/UIBase.hpp>

class EditorUI;

class MenuBar : public UIComponent, public MenuOptionInterface
{
public:
    
    Float _ImGuiMenuHeight = 0.0f;
    EditorUI& _Editor;

    MenuBar(EditorUI& ui);

    virtual Bool Render() final override;
    
protected:
    
    void _OnExportOpen(String file);

    void _OnPlayAnimationSelect(String file);

    void _DoPlayAnimation(std::vector<Symbol>*);

};
