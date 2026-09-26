#include <UI/ApplicationUI.hpp>
#include <UI/UIEditors.hpp>
#include <nfd.h>
#include <imgui.h>
#include <Common/Animation.hpp>
#include <AnimationManager.hpp>

DECL_VEC_ADDITION();

MenuBar::MenuBar(EditorUI& ui) : UIComponent(ui.GetApplication()), _Editor(ui) {}

static constexpr Float MENU_ITEM_HEIGHT = 24.0f;
static constexpr Float MENU_ITEM_SPACING = 4.0f;

static Bool _RenderMenuItem(MenuBar* mb, const String& langID, CString iconTex, Float& xBack, Float wSizeX, Float wSizeY)
{
    xBack -= (MENU_ITEM_HEIGHT + MENU_ITEM_SPACING);
    mb->DrawResourceTexture(iconTex, xBack / wSizeX, 9.f / wSizeY, MENU_ITEM_HEIGHT / wSizeX, MENU_ITEM_HEIGHT / wSizeY);
    Bool hov = ImGui::IsMouseHoveringRect(ImVec2(xBack, 9.0f), ImVec2(xBack + MENU_ITEM_HEIGHT, 9.0f + MENU_ITEM_HEIGHT));
    Bool cl = false;
    if(hov)
    {
        cl = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        ImGui::SetTooltip("%s", mb->_MyUI.GetLanguageText(langID.c_str()).c_str());
    }
    return cl;
}

static Bool _AsyncScriptExec(const JobThread& thread, void* userA, void* userB)
{
    String src = std::move(*((String*)userA));
    String name = std::move(*((String*)userB));
    TTE_DEL((String*)userA); TTE_DEL((String*)userB); // free input arg
    Bool bResult = false;
    
    if((bResult = ScriptManager::LoadChunk(thread.L, name, src)))
    {
        thread.L.CallFunction(0, 0, true);
    }
    
    return bResult;
}

void MenuBar::_DoPlayAnimation(std::vector<Symbol>* singleFile)
{
    TTE_ASSERT(singleFile && singleFile->size() == 1, "Invalid call"); // !!
    if(singleFile && singleFile->size() == 1)
    {
        String targetAgent = _Editor.IsInspectingAgent && _Editor.InspectingNode.lock() ? _Editor.InspectingNode.lock()->AgentName : "";
        if(targetAgent.empty())
        {
            PlatformMessageBoxAndWait("No Selected Agent", _Editor.GetLanguageText("misc.need_agent"));
        }
        else
        {
            Handle<Animation> hAnim{};
            hAnim.SetObject(_MyUI.GetRegistry(), (*singleFile)[0], false, true);
            Ptr<PlaybackController> pController = _Editor.GetActiveScene().PlayAnimation(targetAgent, hAnim.GetObject(_MyUI.GetRegistry(), true), true); // DETACHED
            pController->SetLooping(true);
            pController->Play(); // play!
        }
    }
}

void MenuBar::_OnPlayAnimationSelect(String file) 
{
    Handle<Animation> hAnim{};
    hAnim.SetObject(file);
    if(hAnim.IsLoaded(_MyUI.GetRegistry()))
    {
        std::vector<Symbol> vec{}; vec.push_back(file);
        _DoPlayAnimation(&vec);
    }
    else
    {
        _MyUI.GetRegistry()->PreloadWithCallback({ HandleBase{file} }, false, ALLOCATE_METHOD_CALLBACK_1(this, _DoPlayAnimation, MenuBar, std::vector<Symbol>*), true, EditorUI::PreloadMask);
    }
}

void MenuBar::_OnExportOpen(String file)
{
    nfdchar_t* op=0;
    if(NFD_PickFolder(0, &op, 0) == NFD_OKAY)
    {
        String folder = (CString)op;
        if(!StringEndsWith(folder, "/") && !StringEndsWith(folder, "\\"))
            folder += "/";
        free(op);
        CString error = "The output json file could not be opened!";
        DataStreamRef out = DataStreamManager::GetInstance()->CreateFileStream(folder + file + ".json");
        if(out)
        {
            error = "The requested meta stream file could not be opened!";
            DataStreamRef stream = GetApplication().GetRegistry()->FindResource(file);
            if(stream)
            {
                error = "The file could not be read at the moment. Check the logs!";
                if(Meta::ReadMetaStream(file, stream, out))
                    error = nullptr;
            }
        }
        if(error)
        {
            PlatformMessageBoxAndWait("Could not export to JSON", error);
        }
        else
        {
            TTE_LOG("Dumped JSON for meta stream successfully for %s", file.c_str());
        }
    }
}

Bool MenuBar::Render()
{

    // update a

    if (ImGui::BeginMainMenuBar())
    {
        _ImGuiMenuHeight = ImGui::GetWindowSize().y;
        if (ImGui::BeginMenu("Game"))
        {
            AddMenuOptions("Game");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("File"))
        {
            if(ImGui::MenuItem("Open","Ctrl+O"))
            {
                _Editor.UserRequestOpenFile();
            }
            AddMenuOptions("File");
            // New, Open File, Open
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Editor"))
        {
            AddMenuOptions("Editor");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window"))
        {
            AddMenuOptions("Window");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Scripts"))
        {
            if(ImGui::MenuItem("Run..."))
            {
                nfdchar_t* outp{};
                if (NFD_OpenDialog("lua", NULL, &outp) == NFD_OKAY)
                {
                    DataStreamRef source = DataStreamManager::GetInstance()->CreateFileStream(String(outp));
                    if (source)
                    {
                        String* src = TTE_NEW(String, MEMORY_TAG_TEMPORARY_ASYNC);
                        String* nm = TTE_NEW(String, MEMORY_TAG_TEMPORARY_ASYNC);
                        *src = DataStreamManager::GetInstance()->ReadAllAsString(source);
                        *nm = FileGetName(String(outp));
                        JobDescriptor desc{};
                        desc.AsyncFunction = &_AsyncScriptExec;
                        desc.UserArgA = src;
                        desc.UserArgB = nm;
                        JobScheduler::Instance->Post(desc);
                    }
                    else
                    {
                        TTE_LOG("Cannot open %s: open failed", outp);
                    }
                    free(outp);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Scene"))
        {
            // CREATE SCENE, OPEN SCENE, ADD SCENE, CLEAN SCENE, RECENTS
            ImGui::EndMenu();
        }
        // SCENE, SCRIPTS, CHOREOGRAPHY, AUDIO
        // all most file menus just have open XXX, create XXX + recents
        ImGui::EndMainMenuBar();
    }
    
    // MENU OPTION: GAME
    
    // OPEN PROJECT, RECENT, CHECK FOR UPDATES, REPORT A BUG, OUTPUT, TIMESTEP
    // USER SETTINGS, CONVERT, LOCALIZATIONS, UPDATE PREFERENCES, PACKAGES, WIZARDS,
    // CREATE ARM FILES, .., SAVE GAME, LOAD GAME, QUIT
    
    if(TestMenuOption("File", "Open and Export to JSON", "CTRL + SHIFT + O", ImGuiKey_O, true, true))
    {
        if(!GetApplication()._ActivePopup)
        {
            GetApplication().SetCurrentPopup(TTE_NEW_PTR(ResourcePickerPopup, MEMORY_TAG_EDITOR_UI, "Pick file for JSON export", "*", ALLOCATE_METHOD_CALLBACK_1(this, _OnExportOpen, MenuBar, String)), _Editor);
        }
    }
    if(TestMenuOption("Game", "Switch Project", "", 0, false, false))
    {
        GetApplication()._Flags.Add(ApplicationFlag::WANT_SWITCH_PROJECT);
    }
    if(TestMenuOption("Game", "Quit", "CTRL + SHIFT +  Q", ImGuiKey_Q, true, true))
    {
        GetApplication()._Flags.Add(ApplicationFlag::WANT_QUIT);
    }
    if(TestMenuOption("Game", "Dump Tracked Memory", "", 0, false, false))
    {
        Memory::DumpTrackedMemory();
    }
    if (TestMenuOption("File", "Play Animation on Selected Agent", "", 0, false, false))
    {
        if (!GetApplication()._ActivePopup)
        {
            GetApplication().SetCurrentPopup(TTE_NEW_PTR(PlayAnimationPopup, MEMORY_TAG_EDITOR_UI, "Pick Animation", ALLOCATE_METHOD_CALLBACK_1(this, _OnPlayAnimationSelect, MenuBar, String)), _Editor);
        }
    }
    
    // NENU OPTION: WINDOW
    
    if(TestMenuOption("Window", "Open Console", "", 0, false, false))
    {
        if(!GetApplication()._Flags.Test(ApplicationFlag::CONSOLE_WINDOW_OPEN))
        {
            GetApplication().PushWindow(TTE_NEW_PTR(UIConsole, MEMORY_TAG_EDITOR_UI, GetApplication()));
            GetApplication()._Flags.Add(ApplicationFlag::CONSOLE_WINDOW_OPEN);
        }
    }
    if(TestMenuOption("Window", "Open Memory Tracker", "", 0, false, false))
    {
        if(!GetApplication()._Flags.Test(ApplicationFlag::MEMORY_WINDOW_OPEN))
        {
            GetApplication().PushWindow(TTE_NEW_PTR(UIMemoryTracker, MEMORY_TAG_EDITOR_UI, GetApplication()));
            GetApplication()._Flags.Add(ApplicationFlag::MEMORY_WINDOW_OPEN);
        }
    }

    // SHORTCUT FOR OPEN
    if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyReleased(ImGuiKey_O))
        {
            _Editor.UserRequestOpenFile();
        }
    }

    // BEGIN
    SetNextWindowViewport(0.0f, 0.0f, 0, 0, 1.0f, 0.04f, 0, 40, 0);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, {0.0f, 0.0f, 0.0f, 1.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("#menubar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                                    | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                                    | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar
                                    | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse);

    // BACKGROUND
    ImVec2 size = ImGui::GetWindowSize();
    ImVec2 pos = ImGui::GetWindowPos();
    ImGui::GetWindowDrawList()->AddRectFilledMultiColor(pos, pos + ImVec2{ size.x * 0.5f, size.y }, IM_COL32(120, 14, 57,255), 0xff000000u, 0xff000000u, IM_COL32(120, 14, 57, 255));
    ImGui::GetWindowDrawList()->AddRectFilledMultiColor(pos + ImVec2{ size.x, 0.0f }, pos + ImVec2{ size.x * 0.5f, size.y }, IM_COL32(120, 14, 57, 255), 0xff000000u, 0xff000000u, IM_COL32(120, 14, 57, 255));

    // LOGO AND TITLE BAR
    Float xBack = size.x - 4.0f;
    DrawResourceTexture("LogoSquare.png", 2.f / size.x, 2.f / size.y, 36.f / size.x, 36.f / size.y);
    const String title = "Telltale Editor";
    ImGui::PushFont(GetApplication().GetEditorFont(), 20.0f);
    ImGui::SetCursorPos({ 50.0f, size.y * 0.5f - ImGui::CalcTextSize(title.c_str()).y * 0.5f });
    ImGui::TextUnformatted(title.c_str());
    ImGui::PopFont();
    ImGui::PushFont(GetApplication().GetEditorFont(), 10.0f);
    String proj = GetApplication().GetProjectManager().GetHeadProject()->ProjectName + " [" + GetApplication().GetProjectManager().GetHeadProject()->ProjectFile.filename().string() + "]";
    if(!_Editor.GetActiveScene().GetName().empty())
    {
        proj = proj + " | " + _Editor.GetActiveScene().GetName();
    }
    else if(!_Editor._LoadingAsyncScene.empty())
    {
        proj = proj + " | Loading " + _Editor._LoadingAsyncScene + "...";
    }
    else
    {
        proj = proj + " | No Scene Loaded";
    }
    ImVec2 projNameSize = ImGui::CalcTextSize(proj.c_str());
    ImGui::SetCursorPos({ size.x * 0.5f - 0.5f * projNameSize.x, size.y * 0.5f - projNameSize.y * 0.5f });
    ImGui::TextUnformatted(proj.c_str());
    ImGui::PopFont();

    // END
    ImGui::End();
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(2);
    
    return false;
}
