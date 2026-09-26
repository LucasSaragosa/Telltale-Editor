#include <UI/UIEditors.hpp>
#include <Common/Chore.hpp>
#include <AnimationManager.hpp>
#include <UI/ApplicationUI.hpp>

#include <imgui_internal.h>
DECL_VEC_ADDITION();

template<>
void UIResourceEditor<Chore>::OnExit()
{
}

// LERPS
#define TO_SCREENSPACE_XXX(Xval, boxMin) boxMin.x + ((Xval) - ViewStart) / (ViewEnd - ViewStart) * timelineWidth
#define TO_SCREENSPACE_CLAMPED_XXX(Xval, boxMin, boxMax) ImClamp(resourceBoxMin.x + ((Xval) - ViewStart) / (ViewEnd - ViewStart) * timelineWidth, boxMin.x, boxMax.x)
#define FROM_SCREENSPACE_XXX(Xval, boxMin) ViewStart + ((((Xval) - boxMin.x) / timelineWidth) * (ViewEnd - ViewStart))

#define TO_SCREENSPACE(Xval) TO_SCREENSPACE_XXX(Xval, resourceBoxMin)
#define TO_SCREENSPACE_CLAMPED(Xval) TO_SCREENSPACE_CLAMPED_XXX(Xval, resourceBoxMin, resourceBoxMax)
#define FROM_SCREENSPACE(Xval) FROM_SCREENSPACE_XXX(Xval, resourceBoxMin)

void UIResourceEditorRuntimeData<Chore>::AddAgentResourcePostLoadCallback(const std::vector<Symbol>* resources)
{
    String name{};
    auto it = EditorInstance->PreloadingResourceToAgent.begin();
    for(; it != EditorInstance->PreloadingResourceToAgent.end(); it++)
    {
        if(Symbol(it->first) == (*resources)[0])
        {
            name = it->first;
            break;
        }
    }
    if(!name.empty())
    {
        EditorInstance->PreloadingResourceToAgent.erase(it);
        DoAddAgentResourcePostLoadCallback(std::move(name));
    }
}

void UIResourceEditorRuntimeData<Chore>::DoAddAgentResourcePostLoadCallback(String animFile)
{
    Chore* pChore = EditorInstance->GetCommonObject().get();
    Chore::Agent* pAgent = const_cast<Chore::Agent*>(pChore->GetAgent(SelectedAgent));
    Ptr<ResourceRegistry> reg = EditorInstance->GetApplication().GetRegistry();
    if(animFile == "look")
    {
        Ptr<Procedural_LookAt> pLookAt = TTE_NEW_PTR(Procedural_LookAt, MEMORY_TAG_COMMON_INSTANCE, reg);
        TTE_ATTACH_DBG_STR(pLookAt.get(), "Chore Agent " + pChore->GetName() + "/" + pAgent->Name + " Look At");
        // add procedural look at embed
        Chore::Resource& res = pChore->EmplaceResource("Procedural Look At");
        res.Priority = 1;
        res.Embed = pLookAt;
        res.ResFlags.Add(Chore::Resource::ENABLED);
        res.ResFlags.Add(Chore::Resource::VIEW_GRAPHS);
        res.ResFlags.Add(Chore::Resource::EMBEDDED);
        res.Properties = TelltaleEditor::Get()->CreatePropertySet();
        if(!TelltaleEditor::Get()->TestCapability(GameCapability::UNINHERITED_LOOK_ATS))
        {
            // inherited look ats
            if(!reg->ResourceExists(kProceduralLookAtPropName))
            {
                TTE_LOG("WARNING: When creating procedural look at for %s, the module property set doesn't exist in the resource system: %s", 
                    pAgent->Name.c_str(), kProceduralLookAtPropName.c_str());
            }
            PropertySet::AddParent(res.Properties, kProceduralLookAtPropNameSymbol, reg);
        }
        res.ControlAnimation = TTE_NEW_PTR(Animation, MEMORY_TAG_COMMON_INSTANCE, reg);
        pLookAt->Attach(EditorInstance->GetCommonObject(), res);
        pLookAt->AddToChore(EditorInstance->GetCommonObject(), res);
        pAgent->Resources.push_back((I32)pChore->GetResources().size() - 1);
    }
    else
    {
        WeakPtr<MetaOperationsBucket_ChoreResource> pRes = AbstractMetaOperationsBucket::CreateBucketReference<MetaOperationsBucket_ChoreResource>(reg, animFile, true);
        if (pRes.lock())
        {
            // new resource
            Chore::Resource& res = pChore->EmplaceResource(animFile);
            res.Priority = 1;
            res.ResFlags.Add(Chore::Resource::ENABLED);
            res.ResFlags.Add(Chore::Resource::VIEW_GRAPHS);
            res.Properties = TelltaleEditor::Get()->CreatePropertySet();
            res.ControlAnimation = TTE_NEW_PTR(Animation, MEMORY_TAG_COMMON_INSTANCE, reg);
            pRes.lock()->Attach(EditorInstance->GetCommonObject(), res);
            pRes.lock()->AddToChore(EditorInstance->GetCommonObject(), res);
            pAgent->Resources.push_back((I32)pChore->GetResources().size() - 1);
        }
        else
        {
            TTE_LOG("ERROR: Cannot add '%s' to '%s' as a chore resource, failed to load it!", animFile.c_str(), EditorInstance->FileName.c_str());
        }
    }
}

void UIResourceEditorRuntimeData<Chore>::AddAgentResourceCallback(String animFile)
{
    Chore* pChore = EditorInstance->GetCommonObject().get();
    Chore::Agent* pAgent = const_cast<Chore::Agent*>(pChore->GetAgent(SelectedAgent));
    I32 index = 0;
    Ptr<ResourceRegistry> reg = EditorInstance->GetApplication().GetRegistry();
    for(const auto& res: pChore->GetResources())
    {
        if(res.Name == animFile)
        {
            break;
        }
        index++;
    }
    if(index >= pChore->GetResources().size())
    {
        HandleBase hObject{};
        hObject.SetObject(animFile);
        if(hObject.IsLoaded(reg))
        {
            DoAddAgentResourcePostLoadCallback(animFile);
        }
        else
        {
            PreloadingResourceToAgent[animFile] = SelectedAgent;
            std::vector<HandleBase> handles{};
            handles.push_back(std::move(hObject));
            reg->PreloadWithCallback(std::move(handles), false, ALLOCATE_METHOD_CALLBACK_1(this,
                    AddAgentResourcePostLoadCallback, UIResourceEditorRuntimeData<Chore>, const std::vector<Symbol>*), false, EditorInstance->FileName);
            TTE_LOG("Async loading unloaded resource '%s' to add to chore '%s'...", animFile.c_str(), EditorInstance->FileName.c_str());
        }
    }
    else
    {
        for(const auto idx : pAgent->Resources)
        {
            if(idx == index)
            {
                PlatformMessageBoxAndWait("Error", EditorInstance->GetApplication().GetLanguageText("misc.chore_resource_error"));
                return;
            }
        }
        pAgent->Resources.push_back(index);
    }
}

void UIResourceEditorRuntimeData<Chore>::AddAgentCallback(Meta::ClassInstance stringName)
{
    DoAddAgent(COERCE(stringName._GetInternal(), String));
}

void UIResourceEditorRuntimeData<Chore>::DoAddAgent(String agentName)
{
    Chore* pChore = EditorInstance->GetCommonObject().get();
    if(pChore->GetAgent(agentName))
    {
        PlatformMessageBoxAndWait("Error", EditorInstance->GetLanguageText("misc.chore_agent_error"));
    }
    else
    {
        auto& agent = pChore->EmplaceAgent(agentName);
        agent.Properties = TelltaleEditor::Get()->CreatePropertySet();
    }
}

template<>
Bool UIResourceEditor<Chore>::RenderEditor()
{
    GetApplication().GetRegistry()->Update(0.5f, FileName);
    EditorInstance = this;
    // TABS: FILE, EDIT, VIEW (TIDY = COLLAPSES ALL (CLEAR OPEN MAP), AGENTS, RESOURCES, BEHAVIOUR
    // file: set length, compute length, insert time, add 1 sec, add 5 sec (or use 5/1 keys), sync lang resources
    // per chore agent: agent, resources, filter. resources: add animation, add vox, add aud, (add bank etc), add lang res, add proclook, add other
    // THEY ONLY BLEND WHEN THEY ARE THE SAME PRIORITY
    
    // CHORE TRACKER MENU (CHECK AROUND  TOOL 102_2 MP4 39 MINS

    if(!SubRefFence)
    {
        SubRefFence = TTE_NEW_PTR(U8, MEMORY_TAG_TRANSIENT_FENCE, 0);
    }

    // SUB RESOURCE MGR
    if(!PreloadFinished)
    {
        if(!PreloadAwaiting)
        {
            // Dispatch
            std::vector<HandleBase> preloadHandles{};
            preloadHandles.reserve(GetCommonObject()->GetResources().size());
            for(const auto& resource: GetCommonObject()->GetResources())
            {
                if(!resource.ResFlags.Test(Chore::Resource::EMBEDDED) && !resource.ResFlags.Test(Chore::Resource::AGENT_RESOURCE))
                {
                    HandleBase hResource{};
                    hResource.SetObject(resource.Name);
                    preloadHandles.push_back(hResource);
                }
            }
            if(preloadHandles.empty())
            {
                PreloadFinished = true;
            }
            else
            {
                PreloadAwaiting = true;
                PreloadOffset = GetApplication().GetRegistry()->Preload(std::move(preloadHandles), false);
            }
        }
        else
        {
            if(GetApplication().GetRegistry()->GetPreloadOffset() >= PreloadOffset)
            {
                FailedResources.clear();
                ResourcesCache.clear();
                PreloadAwaiting = false;
                PreloadFinished = true;
            }
        }
    }

    Bool closing = false;
    Ptr<Chore> pChore = GetCommonObject();
    ImGui::SetNextWindowSizeConstraints(ImVec2{ 450.0f, 600.0f }, ImVec2{ 99999.0f, 99999.0f });
    if(ImGui::Begin(GetTitle().c_str(), 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar))
    {
        // SPLIT SECTIONS
        ImGui::GetWindowDrawList()->ChannelsSplit(2);

        ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);
        
        ImVec2 wpos = ImGui::GetWindowPos();
        ImVec2 wsize = ImVec2{ ImGui::GetCurrentWindowRead()->ScrollbarY ? ImGui::GetContentRegionAvail().x + 10.0f : ImGui::GetWindowSize().x, ImGui::GetWindowSize().y };
        CurrentY = 45.0f;

        // SECTION: MENU OPTIONS

        RenderMenuOptions(closing, pChore);
        
        // SECTION: PLAYBACK CONTROLLER

        ImGui::GetWindowDrawList()->AddRectFilled(wpos, wpos + ImVec2{wsize.x, 105.0f}, IM_COL32(45, 45, 45, 255));
        if(ImageButton("Chore/Back.png", 10.0f, CurrentY, 25.0f, 25.0f))
        {
            CurrentTime = ViewStart;
        }
        if(ImageButton("Chore/Stop.png", 40.0f, CurrentY, 25.0f, 25.0f))
        {
            CurrentTime = ViewStart;
            IsPlaying = false;
        }
        if(ImageButton(IsPlaying ? "Chore/Pause.png" : "Chore/Play.png", IsPlaying ? 70.0f : 68.0f,
                       IsPlaying ? CurrentY : CurrentY - 2.0f, IsPlaying ? 25.0f : 29.0f, IsPlaying ? 25.0f : 29.0f))
        {
            IsPlaying = !IsPlaying;
        }
        if(ImageButton("Chore/Forward.png", 100.0f, CurrentY, 25.0f, 25.0f))
        {
            IsPlaying = true; // reset and play from start
            CurrentTime = ViewStart;
        }
        SelectionBox(160.0f, CurrentY, 25.0f, 25.0f, ExtraDataOpen);
        if(ImageButton("PropertySet/AnimMonitor.png", 160.0f, CurrentY, 25.0f, 25.0f))
        {
            ExtraDataOpen = !ExtraDataOpen;
        }
        SelectionBox(200.0f, CurrentY, 25.0f, 25.0f, IsLooping);
        if(ImageButton("Chore/Loop.png", 200.0f, CurrentY, 25.0f, 25.0f))
        {
            IsLooping = !IsLooping;
        }

        Float choreLen = GetCommonObject()->GetLength();
        if (choreLen <= 0.000001f) choreLen = 0.00001f; // avoid divide by zero


        // Fix invalid end range
        if (ViewEnd <= ViewStart && choreLen > 0.000001f)
        {
            ViewEnd = choreLen;
        }

        // length
        char Temp[16]{0};
        Float textStartLen = 0.0f;
        snprintf(Temp, 16, "%.2f", CurrentTime);
        ImGui::PushFont(ImGui::GetFont(), 25.f);
        Float size = ImGui::CalcTextSize(Temp).x;
        ImGui::SetCursorScreenPos(wpos + ImVec2{textStartLen = wsize.x - 10.0f - size, 45.0f});
        ImGui::TextUnformatted(Temp);
        ImGui::PopFont();

        if (PreloadAwaiting || !PreloadingResourceToAgent.empty())
        {
            const String& langTextLoading = GetLanguageText(PreloadingResourceToAgent.empty() ? "misc.loading_resource" : "misc.loading_chore");
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(196, 94, 35, 255));
            ImGui::PushFont(ImGui::GetFont(), 13.0f);
            ImGui::SetCursorScreenPos(wpos + ImVec2{ textStartLen - ImGui::CalcTextSize(langTextLoading.c_str()).x - 50.0f, CurrentY + 7.0f });
            if(PreloadingResourceToAgent.empty())
            {
                ImGui::Text("%s%s", langTextLoading.c_str(), PreloadAnimator.GetEllipses().c_str());
            }
            else
            {
                ImGui::Text("%s (%d)", langTextLoading.c_str(), (U32)PreloadingResourceToAgent.size());
            }
            ImGui::PopFont();
            ImGui::PopStyleColor();
        }

        CurrentY += 30.f;

        // =====================================================================================================================================================================
        // ===================================================================== CHORE POSITION SCRUBBER =======================================================================
        // =====================================================================================================================================================================

        // konst
        const Float RES_HEIGHT = 30.0F;
        const Float margin = 5.0f;
        const Float sliderHeight = 25.0f;
        const Float backBarHeight = 15.0f;
        const Float viewBarHeight = 22.0f;
        const Float handleWidth = 6.0f;
        const Float minPixelWidth = 30.0f;

        // pos
        ImVec2 sliderMin{ margin, CurrentY };
        ImVec2 sliderMax{ wsize.x - margin, CurrentY + sliderHeight };
        ImVec2 sliderBackPos{ margin, CurrentY + (sliderHeight - backBarHeight) * 0.5f };
        ImVec2 sliderBackSize{ wsize.x - (margin * 2), backBarHeight };

        // bg
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(wpos + sliderBackPos, wpos + sliderBackPos + sliderBackSize, IM_COL32(80, 80, 80, 255));
        dl->AddRect(wpos + sliderBackPos, wpos + sliderBackPos + sliderBackSize, IM_COL32(100, 100, 100, 255));

        // lerps
        Float usableWidth = sliderBackSize.x;
        Float startX = sliderBackPos.x + (ViewStart / choreLen) * usableWidth;
        Float endX = sliderBackPos.x + (ViewEnd / choreLen) * usableWidth;

        // grab checks
        Float distancePixels = endX - startX;
        Float distanceTime = ViewEnd - ViewStart;
        if (distancePixels < minPixelWidth && distanceTime > 0.0f)
        {
            Float scale = minPixelWidth / distancePixels;
            Float newDistanceTime = distanceTime * scale;
            Float center = (ViewStart + ViewEnd) * 0.5f;

            ViewStart = center - newDistanceTime * 0.5f;
            ViewEnd = center + newDistanceTime * 0.5f;

            if (ViewStart < 0.0f)
            {
                ViewEnd -= ViewStart;
                ViewStart = 0.0f;
            }
            if (ViewEnd > choreLen)
            {
                Float diff = ViewEnd - choreLen;
                ViewStart -= diff;
                ViewEnd = choreLen;
            }
        }

        // clamps
        Float maxOffset = handleWidth * 0.5f;
        startX = ImClamp(startX, sliderBackPos.x + maxOffset, sliderBackPos.x + sliderBackSize.x - maxOffset);
        endX = ImClamp(endX, sliderBackPos.x + maxOffset, sliderBackPos.x + sliderBackSize.x - maxOffset);
        if (endX < startX) endX = startX + 1.0f;

        // active region
        Float viewY = CurrentY + (sliderHeight - viewBarHeight) * 0.5f;
        ImVec2 rangeMin = { startX, viewY };
        ImVec2 rangeMax = { endX,   viewY + viewBarHeight };
        ImVec2 activeRegionInnerMin = rangeMin - ImVec2{ handleWidth * 0.5f, 0.0f } + ImVec2{ 5.0f, 5.0f };
        ImVec2 activeRegionInnerMax = rangeMax + ImVec2{ handleWidth * 0.5f, 0.0f } - ImVec2{ 5.0f, 5.0f };
        dl->AddRectFilled(wpos + rangeMin - ImVec2{ handleWidth * 0.5f, 0.0f }, wpos + rangeMax + ImVec2{ handleWidth * 0.5f, 0.0f }, IM_COL32(60, 60, 60, 255));
        dl->AddRectFilled(wpos + activeRegionInnerMin, wpos + activeRegionInnerMax, IM_COL32(200, 200, 200, 255));

        // current time
        Float curX = sliderBackPos.x + (CurrentTime / choreLen) * usableWidth;
        curX = ImClamp(
            curX,
            activeRegionInnerMin.x + 5.0f,   // small inset so it doesn’t touch the border
            activeRegionInnerMax.x - 5.0f
        );
        //curX = ImClamp(curX, sliderBackPos.x + maxOffset, sliderBackPos.x + sliderBackSize.x - maxOffset);
        ImVec2 curMin{ curX - 5.0f, viewY + 5.0f };
        ImVec2 curMax{ curX + 5.0f, viewY + viewBarHeight - 5.0f };
        dl->AddRectFilled(wpos + curMin, wpos + curMax, IM_COL32(50, 150, 50, 255));

        // mouse
        ImVec2 mouse = ImGui::GetMousePos();
        Float localX = mouse.x - wpos.x;
        Bool leftClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        // click grab
        if (leftClicked) 
        {
            SliderGrabbed = ViewStartGrabbed = ViewEndGrabbed = false;

            if (ImGui::IsMouseHoveringRect(wpos + curMin, wpos + curMax, false))
                SliderGrabbed = true;
            else if (ImGui::IsMouseHoveringRect(wpos + ImVec2{ startX - handleWidth, CurrentY + 5.0f },
                wpos + ImVec2{ startX + handleWidth, CurrentY + 5.0f + viewBarHeight }, false))
                ViewStartGrabbed = true;
            else if (ImGui::IsMouseHoveringRect(wpos + ImVec2{ endX - handleWidth, CurrentY + 5.0f },
                wpos + ImVec2{ endX + handleWidth, CurrentY + 5.0f + viewBarHeight }, false))
                ViewEndGrabbed = true;
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) 
        {
            SliderGrabbed = ViewStartGrabbed = ViewEndGrabbed = false;
        }

        // update pos
        if (ViewStartGrabbed)
        {
            float t = ImClamp((localX - sliderBackPos.x) / usableWidth, 0.0f, 1.0f);
            ViewStart = t * choreLen;
            if (ViewStart > ViewEnd - 0.001f) ViewStart = ViewEnd - 0.001f;
        }

        if (ViewEndGrabbed) 
        {
            float t = ImClamp((localX - sliderBackPos.x) / usableWidth, 0.0f, 1.0f);
            ViewEnd = t * choreLen;
            if (ViewEnd < ViewStart + 0.001f) ViewEnd = ViewStart + 0.001f;
        }

        if (SliderGrabbed) 
        {
            float t = ImClamp((localX - sliderBackPos.x) / usableWidth, 0.0f, 1.0f);
            CurrentTime = t * choreLen;
        }
        CurrentTime = ImClamp(CurrentTime, ViewStart, ViewEnd); // always ensure this

        CurrentY = 105.0f; // start of rest of the data

        
        // =====================================================================================================================================================================
        // ============================================================================= TOOL BAR ==============================================================================
        // =====================================================================================================================================================================

        {
            ImGui::GetWindowDrawList()->AddRectFilled(wpos + ImVec2{ 0.0f, CurrentY }, wpos + ImVec2{ wsize.x, CurrentY + 35.0f }, IM_COL32(30, 30, 30, 255));

            Float curX = 5.0f;
            SelectionBox(curX, CurrentY + 5.0f, 26.0f, 26.0f, false); // for highlight
            if(ImageButton("Chore/ChoreAdd1.png", curX, CurrentY + 5.0f, 26.0f, 26.0f))
            {
                pChore->_Length += 1.0f;
                ViewEnd += 1.0f;
            }
            curX += 30.0f;
            SelectionBox(curX, CurrentY + 5.0f, 26.0f, 26.0f, false);
            if (ImageButton("Chore/ChoreAdd5.png", curX, CurrentY + 5.0f, 26.0f, 26.0f))
            {
                pChore->_Length += 5.0f;
                ViewEnd += 5.0f;
            }
            curX += 30.0f;

            CurrentY += 35.0f;
        }

        // ======================================================================================================================================================================
        // ============================================================================= EXTRA DATA =============================================================================
        // ======================================================================================================================================================================

        if(ExtraDataOpen)
        {
            ImGui::GetWindowDrawList()->AddRectFilled(wpos + ImVec2{ 0.0f, CurrentY }, wpos + ImVec2{ wsize.x, CurrentY + 74.0f }, IM_COL32(60, 60, 60, 255));

            ImGui::BeginDisabled();
            ImGui::PushItemWidth(250.0f);
            ImGui::PushFont(ImGui::GetFont(), 10.0f);

            ImGui::SetCursorScreenPos(wpos + ImVec2{ 20.0f, CurrentY + 5.0f });
            ImGui::TextUnformatted("Length");
            ImGui::SetCursorScreenPos(wpos + ImVec2{ 160.0f, CurrentY + 2.0f });
            ImGui::InputFloat("##Len", &choreLen);

            ImGui::SetCursorScreenPos(wpos + ImVec2{ 20.0f, CurrentY + 30.0f });
            ImGui::TextUnformatted("View Begin");
            ImGui::SetCursorScreenPos(wpos + ImVec2{ 160.0f, CurrentY + 27.0f });
            ImGui::InputFloat("##ViewB", &ViewStart);

            ImGui::SetCursorScreenPos(wpos + ImVec2{ 20.0f, CurrentY + 55.0f });
            ImGui::TextUnformatted("View End");
            ImGui::SetCursorScreenPos(wpos + ImVec2{ 160.0f, CurrentY + 52.0f });
            ImGui::InputFloat("##ViewE", &ViewEnd);

            ImGui::PopFont();
            ImGui::PopItemWidth();
            ImGui::EndDisabled();

            CurrentY += 74.0f;
        }

        ImGui::GetWindowDrawList()->AddRectFilled(wpos + ImVec2{ 0.0f, CurrentY }, wpos + ImVec2{ wsize.x, CurrentY + 3.0f }, IM_COL32(30, 30, 30, 255));
        CurrentY += 3.0f; // divider

        // =====================================================================================================================================================================
        // ======================================================================= CHORE TIMELINE AREA =========================================================================
        // =====================================================================================================================================================================       

        ImGui::GetWindowDrawList()->ChannelsSetCurrent(0);
        RenderChoreTimeline(pChore, RES_HEIGHT, wpos, wsize, leftClicked);
        
    }
    ImGui::End();
    return closing;
}

void UIResourceEditorRuntimeData<Chore>::RenderChoreResourceBlock(const Ptr<Chore>& pChore, Chore::Resource& resource, Chore::Agent& agent, _RenderResourceBlockState& bstate)
{
    const Float timelineWidth = bstate.resourceBoxMax.x - bstate.resourceBoxMin.x;
    Float timeStart = bstate.block.Start;
    Float timeEnd = bstate.block.Start + (bstate.block.End - bstate.block.Start) * bstate.block.Scale;

    Float xStart = TO_SCREENSPACE_XXX(timeStart, bstate.resourceBoxMin);
    Float xEnd = TO_SCREENSPACE_XXX(timeEnd, bstate.resourceBoxMin);

    if (xEnd < bstate.resourceBoxMin.x || xStart > bstate.resourceBoxMax.x)
    {
        // Out of view
    }
    else
    {
        xStart = MAX(xStart, bstate.resourceBoxMin.x);
        xEnd = MIN(xEnd, bstate.resourceBoxMax.x);

        ImVec2 blockMin = ImVec2{ xStart, bstate.resourceBoxMin.y + 2.0f };
        ImVec2 blockMax = ImVec2{ xEnd,   bstate.resourceBoxMax.y - 2.0f };

        ImGui::GetWindowDrawList()->AddRectFilled(
            blockMin, blockMax,
            bstate.block.Looping ? IM_COL32(199, 26, 138, 255) : IM_COL32(68, 141, 184, 180),
            2.0f
        );

        //  FORCE LOGIC FOR LOOPING BLOCKS
        if (bstate.block.Looping)
        {
            bstate.block.Scale = 1.0f;
            Float curLen = bstate.block.End - bstate.block.Start;
            if (curLen < bstate.resourceLength)
                bstate.block.End = bstate.block.Start + bstate.resourceLength;
        }

        //  SELECTED BLOCK OUTLINE
        if (bstate.pSelectedBlock)
        {
            ImGui::GetWindowDrawList()->AddRect(blockMin, blockMax, IM_COL32(72, 86, 94, 255), 3.0f, 0, 2.0f);
        }

        //  HANDLE DRAWING (LOOPING ALWAYS SHOWS)
        Bool showHandles = bstate.block.Looping || (bstate.pSelectedBlock && ScaleMode);

        if (showHandles)
        {
            ImU32 col = bstate.block.Scale == 1.0f ? IM_COL32(100, 100, 100, 255) : bstate.block.Scale > 1.0f
                ? IM_COL32(217, 69, 11, 255) : IM_COL32(89, 219, 13, 255);
            ImGui::GetWindowDrawList()->AddCircleFilled(blockMin + ImVec2(2, 2), 4, col);
            ImGui::GetWindowDrawList()->AddCircleFilled(blockMax - ImVec2(2, 2), 4, col);
            ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2{ blockMin.x, blockMax.y } + ImVec2(2, -2), 4, col);
            ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2{ blockMax.x, blockMin.y } + ImVec2(-2, 2), 4, col);
        }

        //  INTERACTION LOGIC
        if (bstate.pSelectedBlock)
        {
            if (bstate.state.rs.mouseClickedThisFrame)
            {
                if (ImGui::IsMouseHoveringRect(blockMin, ImVec2{ blockMin.x + 4, blockMax.y }, false))
                {
                    ActiveScaleMode = START;
                    bstate.state.rs.anythingClicked = true;
                }
                else if (ImGui::IsMouseHoveringRect(ImVec2{ blockMax.x - 4, blockMin.y }, blockMax, false))
                {
                    ActiveScaleMode = END;
                    bstate.state.rs.anythingClicked = true;
                }
            }

            Float deltaTime = (bstate.state.rs.mouseDeltaX / timelineWidth) * (ViewEnd - ViewStart);

            if (bstate.state.rs.mouseDown)
            {
                if (bstate.block.Looping)
                {
                    Float curLen = bstate.block.End - bstate.block.Start;

                    if (ActiveScaleMode == START)
                    {
                        Float newStart = bstate.block.Start + deltaTime;
                        if (bstate.block.End - newStart < bstate.resourceLength)
                            newStart = bstate.block.End - bstate.resourceLength;

                        bstate.block.Start = newStart;
                    }
                    else if (ActiveScaleMode == END)
                    {
                        Float newEnd = bstate.block.End + deltaTime;
                        if (newEnd - bstate.block.Start < bstate.resourceLength)
                            newEnd = bstate.block.Start + bstate.resourceLength;

                        bstate.block.End = newEnd;
                    }
                    else
                    {
                        bstate.block.Start = ImClamp(bstate.block.Start + deltaTime, 0.0f, pChore->_Length - curLen);
                        bstate.block.End = bstate.block.Start + curLen;
                    }
                }
                else if (ScaleMode)
                {
                    Float baseLen = (bstate.block.End - bstate.block.Start);
                    Float origLen = baseLen * bstate.block.Scale;

                    if (ActiveScaleMode == START)
                    {
                        Float origEnd = bstate.block.Start + origLen;

                        Float newLength = origLen - deltaTime;
                        newLength = ImClamp(
                            newLength,
                            (20.0f / timelineWidth) * (ViewEnd - ViewStart),
                            ViewEnd - ViewStart
                        );

                        Float newStart = origEnd - newLength;

                        bstate.block.Start = ImClamp(newStart, 0.0f, origEnd - 0.001f);
                        bstate.block.Scale = newLength / baseLen;
                        bstate.block.End = bstate.block.Start + baseLen;
                    }
                    else if (ActiveScaleMode == END)
                    {
                        Float newLength = origLen + deltaTime;
                        newLength = ImClamp(
                            newLength,
                            (20.0f / timelineWidth) * (ViewEnd - ViewStart),
                            ViewEnd - ViewStart
                        );

                        bstate.block.Scale = newLength / baseLen;
                        bstate.block.End = bstate.block.Start + baseLen;
                    }
                }
                else
                {
                    Float baseLen = (bstate.block.End - bstate.block.Start);
                    Float scaledLen = baseLen * bstate.block.Scale;
                    bstate.block.Start = ImClamp(bstate.block.Start + deltaTime, 0.0f, pChore->_Length - scaledLen);
                    bstate.block.End = bstate.block.Start + baseLen;
                }
            }
        }
        Bool bNeedSelect = bstate.pSelectedBlock == nullptr && SelectionBoxReady && ImRect{ bstate.state.rs.selectionRectMin, bstate.state.rs.selectionRectMax }.Overlaps(ImRect{ blockMin, blockMax });
        Bool blockHov = ImGui::IsMouseHoveringRect(blockMin, blockMax, false);
        if (blockHov && bstate.state.rs.mouseClickedThisFrame)
        {
            bstate.state.rs.anythingClicked = true;
            if (SelectionBoxReclickAvail && !bstate.state.rs.usedSelectionReclick)
            {
                bstate.state.rs.usedSelectionReclick = true;
            }
            else
            {
                if (bstate.pSelectedBlock)
                {
                    if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
                    {
                        for (auto it = SelectedResourceBlocks.begin(); it != SelectedResourceBlocks.end();)
                        {
                            if (*it == *bstate.pSelectedBlock)
                            {
                                it = SelectedResourceBlocks.erase(it);
                                bstate.pSelectedBlock = nullptr;
                                break;
                            }
                            else
                            {
                                it++;
                            }
                        }
                    }
                }
                else
                {
                    if (!ImGui::IsKeyDown(ImGuiKey_LeftShift))
                    {
                        SelectedResourceBlocks.clear();
                    }
                    bNeedSelect = true;
                }
            }
        }
        if (bNeedSelect)
        {
            SelectedResourceBlocks.push_back({ resource.Name, bstate.currentBlockNumber });
        }
    }
}

void UIResourceEditorRuntimeData<Chore>::RenderChoreResource(const Ptr<Chore>& pChore, Chore::Resource& resource, Chore::Agent& agent, _RenderResourceState& state)
{

    // =====================================================================================================================================================================
    // ======================================================================= AWAIT RESOURCE PRELOAD ======================================================================
    // =====================================================================================================================================================================      
    
    MetaOperationsBucket_ChoreResource _dummyAgentResourceBucket{};
    Ptr<MetaOperationsBucket_ChoreResource> resourceInterface{};
    Symbol resNameSymbol = resource.Name;
    auto rcacheIterator = ResourcesCache.find(resource.Name);
    Bool bFail = false;
    if (PreloadAwaiting)
    {
        bFail = true;
    }
    else
    {
        Bool bEmbed = resource.ResFlags.Test(Chore::Resource::EMBEDDED);
        Bool bAgentResource = resource.ResFlags.Test(Chore::Resource::AGENT_RESOURCE);
        if (bAgentResource)
        {
            // dummy. agent resource directly maps to the scene agent itself, control anim etc
            resourceInterface = TTE_PROXY_PTR(&_dummyAgentResourceBucket, MetaOperationsBucket_ChoreResource);
        }
        else
        {
            if (bEmbed)
            {
                resourceInterface = resource.Embed;
                bFail = resourceInterface == nullptr;
                if (bFail && FailedResources.find(resource.Name) == FailedResources.end())
                {
                    FailedResources.insert(resource.Name);
                    TTE_LOG("ERROR: Embedded chore resource '%s' for chore '%s' is empty!", resource.Name.c_str(), pChore->_Name.c_str());
                }
                else if (resourceInterface != nullptr && rcacheIterator == ResourcesCache.end())
                {
                    ResourcesCache[resource.Name] = resourceInterface;
                }
            }
            else if (rcacheIterator == ResourcesCache.end())
            {
                if (FailedResources.find(resource.Name) != FailedResources.end())
                    bFail = true;
                else
                {
                    WeakPtr<MetaOperationsBucket_ChoreResource> pResource =
                        AbstractMetaOperationsBucket::CreateBucketReference<MetaOperationsBucket_ChoreResource>(EditorInstance->GetApplication().GetRegistry(), resource.Name, false);
                    if (pResource.lock())
                    {
                        ResourcesCache[resource.Name] = std::move(pResource);
                    }
                    else
                    {
                        bFail = true;
                        FailedResources.insert(resource.Name);
                        TTE_LOG("ERROR: Chore resource '%s' for chore '%s' could not be loaded/found, or is not a chore resource operations bucket yet!", resource.Name.c_str(), pChore->_Name.c_str());
                    }
                }
            }
            if (!bFail && !bEmbed)
            {
                if (rcacheIterator == ResourcesCache.end())
                    rcacheIterator = ResourcesCache.find(resource.Name);
                if (rcacheIterator == ResourcesCache.end() || rcacheIterator->second.expired())
                {
                    WeakPtr<MetaOperationsBucket_ChoreResource> pResource =
                        AbstractMetaOperationsBucket::CreateBucketReference<MetaOperationsBucket_ChoreResource>(EditorInstance->GetApplication().GetRegistry(), resource.Name, false);
                    if (pResource.lock())
                    {
                        rcacheIterator->second = std::move(pResource);
                    }
                    else
                    {
                        bFail = true;
                        if (rcacheIterator != ResourcesCache.end())
                            ResourcesCache.erase(rcacheIterator);
                        TTE_LOG("WARNING: Chore resource '%s' was unloaded and could not be reloaded! Disabling this resource...", resource.Name.c_str(), pChore->_Name.c_str());
                    }
                }
            }
        }
    }
    if (!resourceInterface)
        resourceInterface = bFail ? nullptr : ResourcesCache[resource.Name].lock();

    // =====================================================================================================================================================================
    // ========================================================================= RESOURCE BOX DRAW =========================================================================
    // =====================================================================================================================================================================      

    Vector3 paramColour{ 219.0f / 255.0f, 4.0f / 255.0f, 4.0f / 255.0f };
    CString resicon = "Chore/Unknown.png";
    Float resourceLength = 0.001f;
    if (!bFail)
    {
        resourceInterface->GetRenderParameters(paramColour, resicon);
        resourceLength = resourceInterface->GetLength();
    }
    Bool bResourceSelected = SelectedResourceIndex == state.agentResourceIndex && SelectedAgent == agent.Name;
    ImGui::GetWindowDrawList()->AddRectFilled(state.rs.wpos + ImVec2{ 0.0f, CurrentY }, state.rs.wpos + ImVec2{ state.rs.wsize.x, CurrentY + state.rs.RES_HEIGHT + 2.0f }, IM_COL32(205, 212, 201, 255));
    ImVec2 resourceBoxMin = state.rs.wpos + ImVec2{ 6.0f, CurrentY };
    ImVec2 resourceBoxMax = state.rs.wpos + ImVec2{ state.rs.wsize.x - 6.0f, CurrentY + state.rs.RES_HEIGHT };
    ImGui::GetWindowDrawList()->AddRectFilled(resourceBoxMin, resourceBoxMax, ImColor(paramColour.x, paramColour.y, paramColour.z, 1.0f));
    ImGui::GetWindowDrawList()->AddRect(resourceBoxMin, resourceBoxMax,
        bResourceSelected ? IM_COL32(110, 104, 71, 255) : IM_COL32(79, 181, 209, 255), 0.0f, 0, bResourceSelected ? 2.0f : 0.0f);
    if (ImGui::IsMouseHoveringRect(resourceBoxMin, resourceBoxMax, false) && (state.rs.mouseClickedThisFrame || state.rs.mouseRightReleased))
    {
        SelectedAgent = agent.Name;
        SelectedResourceIndex = state.agentResourceIndex;
        state.rs.anythingClicked = true;
        bResourceSelected = true;
    }
    if (bResourceSelected)
        OpenContextMenu("resource", resourceBoxMin, resourceBoxMax);
    resourceBoxMin.x += 3.0f;
    resourceBoxMax.x -= 3.0f;
    resourceBoxMin.y += 2.0f;
    resourceBoxMax.y -= 2.0f; // make the rest in a slightly smaller inner box

    // =======================================================================================================================================================================
    // ========================================================================= RIGHT CLICK OPTIONS =========================================================================
    // ======================================================================================================================================================================= 

    Bool bCreateBlock = false, bCreateFades = true, bDeleteBlock = false, bToggleLoop = false;
    if (bResourceSelected && TestMenuOption("resource", "Disable", "", 0, false, false, false, resource.ResFlags.Test(Chore::Resource::ENABLED) ? nullptr : "Enable"))
    {
        resource.ResFlags.Toggle(Chore::Resource::ENABLED);
    }
    else if (bResourceSelected && TestMenuOption("resource", "Reset Length", "", 0, false, false, true))
    {
        for (auto& block : resource.Blocks)
        {
            block.Scale = 1.0f; // reset all scales
        }
    }
    else if (bResourceSelected && TestMenuOption("resource", "Raise Priority", "[PAGE UP]", ImGuiKey_PageUp, false, false))
    {
        resource.Priority++;
    }
    else if (bResourceSelected && TestMenuOption("resource", "Lower Priority", "[PAGE DOWN]", ImGuiKey_PageDown, false, false, true))
    {
        resource.Priority--;
    }
    // TODO CREATE GUIDE?
    else if (bResourceSelected && TestMenuOption("resource", "View Graphs", "[G]", ImGuiKey_G, false, false, false, resource.ResFlags.Test(Chore::Resource::VIEW_GRAPHS) ? "Hide Graphs" : nullptr))
    {
        resource.ResFlags.Toggle(Chore::Resource::VIEW_GRAPHS);
    }
    else if (bResourceSelected && TestMenuOption("resource", "View Properties", "[P]", ImGuiKey_P, false, false, false, resource.ResFlags.Test(Chore::Resource::VIEW_PROPERTIES) ? "Hide Properties" : nullptr))
    {
        resource.ResFlags.Toggle(Chore::Resource::VIEW_PROPERTIES);
        String propName = "\"" + resource.Name + "\" Chore Properties";
        if (resource.ResFlags.Test(Chore::Resource::VIEW_PROPERTIES))
        {
            EditorInstance->_EditorUI.DispatchEditorImmediate(TTE_NEW_PTR(UIPropertySet, MEMORY_TAG_EDITOR_UI, propName, EditorInstance->_EditorUI, propName, resource.Properties, WeakPtr<U8>(SubRefFence)));
        }
        else
        {
            // Close
            EditorInstance->_EditorUI.CloseEditor(propName);
        }
    }
    else if (bResourceSelected && TestMenuOption("resource", "View Resource Groups", "[R]", ImGuiKey_R, false, false, true, resource.ResFlags.Test(Chore::Resource::VIEW_GROUPS) ? nullptr : "Hide Resource Groups"))
    {
        resource.ResFlags.Toggle(Chore::Resource::VIEW_GROUPS); // TODO
    }
    // TODO TRIM [T] MODE
    else if (bResourceSelected && TestMenuOption("resource", "Enter Scale Mode", "[S]", ImGuiKey_S, false, false, true,
        ScaleMode ? "Exit Scale Mode" : nullptr))
    {
        ScaleMode = !ScaleMode;
    }
    else if (bResourceSelected && TestMenuOption("resource", "Create Block", "[INSERT]", ImGuiKey_Insert, false, false, false))
    {
        bCreateBlock = true;
    }
    else if (bResourceSelected && TestMenuOption("resource", "Create Block w/o fades", "[SHIFT + INSERT]", ImGuiKey_Insert, true, false, false))
    {
        bCreateBlock = true;
        bCreateFades = false;
    }
    else if (bResourceSelected && TestMenuOption("resource", "Delete Block", "[DELETE]", ImGuiKey_Delete, false, false, false))
    {
        bDeleteBlock = true;
    }
    else if (bResourceSelected && TestMenuOption("resource", "Loop Block", "", ImGuiKey_None, false, false, true))
    {
        bToggleLoop = true;
    }
    // NEXT: toggle posed, filter out mover data [M], disable lip sync abunatuib [Y], toggle play as music,
    // swap out (ctrlw)
    else if (bResourceSelected && TestMenuOption("resource", "Remove this resource", "[CTRL + X]", ImGuiKey_X, false, true, true))
    {
        state.rmResource = state.resourceIndex;
        SelectedResourceIndex = -1;
    }
    // dup resource (Ctrld), add style res group, groups

       // =====================================================================================================================================================================
    // ========================================================================= CREATE RES BLOCK UI ==========================================================================
    // ========================================================================================================================================================================

    if (bCreateBlock)
    {
        Chore::Resource::Block& newBlock = resource.Blocks.emplace_back();
        newBlock.Looping = false;
        newBlock.Scale = 1.0f;
        newBlock.Start = CurrentTime;
        newBlock.End = newBlock.Start + resourceLength;
        if (newBlock.End > ViewEnd)
        {
            ViewEnd = newBlock.End;
        }
        if (ViewEnd > pChore->_Length)
        {
            pChore->_Length = ViewEnd;
        }
        Ptr<KeyframedValue<Float>> pContribution{}, pTime{};
        for (auto& val : resource.ControlAnimation->GetAnimatedValues())
        {
            if (val->GetName() == "time")
            {
                pTime = std::dynamic_pointer_cast<KeyframedValue<Float>>(val);
            }
            else if (val->GetName() == "contribution")
            {
                pContribution = std::dynamic_pointer_cast<KeyframedValue<Float>>(val);
            }
        }
        if (pTime)
        {
            pTime->InsertSample(newBlock.Start, 0.0f);
            pTime->InsertSample(newBlock.End, 1.0f);
        }
        if (pContribution)
        {
            if (bCreateFades)
            {
                pContribution->InsertSample(newBlock.Start, 0.0f);
                pContribution->InsertSample(newBlock.Start + resourceLength * 0.1f, 1.0f);
                pContribution->InsertSample(newBlock.Start + resourceLength * 0.9f, 1.0f);
                pContribution->InsertSample(newBlock.End, 0.0f);
            }
            else
            {
                pContribution->InsertSample(newBlock.Start, 1.0f);
                pContribution->InsertSample(newBlock.End, 1.0f);
            }
        }
        SelectedKeyframeSamples.clear(); // memory changed!
    }

    // ======================================================================================================================================================================
    // ========================================================================= RESOURCE BLOCKS UI =========================================================================
    // ====================================================================================================================================================================== 

    I32 topHighlightedBlock = -1;
    for (auto& block : resource.Blocks)
    {
        _RenderResourceBlockState bstate{ state, resourceBoxMin, resourceBoxMax, block, resourceLength };
        bstate.currentBlockNumber = 0;
        bstate.pSelectedBlock = nullptr;
        for (const auto& selectedBlock : SelectedResourceBlocks)
        {
            if (selectedBlock.Resource == resNameSymbol && bstate.currentBlockNumber == selectedBlock.Index)
            {
                bstate.pSelectedBlock = &selectedBlock;
                topHighlightedBlock = bstate.currentBlockNumber;
                break;
            }
        }
        RenderChoreResourceBlock(pChore, resource, agent, bstate);
        bstate.currentBlockNumber++;
    }
    if (topHighlightedBlock != -1)
    {
        if (bDeleteBlock)
        {
            resource.Blocks.erase(resource.Blocks.begin() + topHighlightedBlock);
            for (auto sit = SelectedResourceBlocks.begin(); sit != SelectedResourceBlocks.end();)
            {
                if (sit->Resource == resNameSymbol)
                {
                    if (sit->Index == topHighlightedBlock)
                    {
                        sit = SelectedResourceBlocks.erase(sit);
                        continue;
                    }
                    else if (sit->Index > topHighlightedBlock)
                    {
                        sit->Index--;
                        continue;
                    }
                }
                sit++;
            }
        }
        else if (bToggleLoop)
        {
            resource.Blocks[topHighlightedBlock].Looping = resource.Blocks[topHighlightedBlock].Looping != 0 ? 0 : 1;
        }
    }
    ImGui::PushFont(ImGui::GetFont(), 12.0f);
    ImVec2 tSize = ImGui::CalcTextSize(resource.Name.c_str());
    ImGui::PushID(state.runningID++);
    EditorInstance->DrawResourceTexturePixels(resicon, state.rs.wsize.x - 30.0f, CurrentY + 5.0f, 21.0f, 21.0f);
    ImGui::PopID();
    ImGui::SetCursorScreenPos(state.rs.wpos + ImVec2{ state.rs.wsize.x - tSize.x - 33.f, CurrentY + 12.0f });
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
    ImGui::TextUnformatted(resource.Name.c_str());
    ImGui::PushFont(ImGui::GetFont(), 16.0f);
    ImGui::SetCursorScreenPos(state.rs.wpos + ImVec2{ 10.f, CurrentY + 7.0f });
    ImGui::Text("Priority: %d", resource.Priority);
    ImGui::PopFont();
    ImGui::PopStyleColor();
    ImGui::PopFont();
    const Float timelineWidth = resourceBoxMax.x - resourceBoxMin.x;
    Float currentX = ImClamp(TO_SCREENSPACE(CurrentTime), resourceBoxMin.x, resourceBoxMax.x);
    if (currentX + 2.0f <= resourceBoxMax.x)
    {
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2{ currentX, state.rs.wpos.y + CurrentY }, ImVec2{ currentX + 4.0f, state.rs.wpos.y + CurrentY + state.rs.RES_HEIGHT }, IM_COL32(100, 100, 100, 190));
    }
    CurrentY += state.rs.RES_HEIGHT + 2.0f;

    // RENDER GRAPHS
    RenderChoreGraphs(resource, *EditorInstance, state.rs, state.runningID);
}

void UIResourceEditorRuntimeData<Chore>::RenderChoreAgent(const Ptr<Chore>& pChore, Chore::Agent& agent, _RenderState& rs, Bool leftClicked)
{
    // CHORE AGENT BOX
    Bool open = OpenChoreAgents.find(agent.Name) != OpenChoreAgents.end();
    ImGui::GetWindowDrawList()->AddRectFilled(rs.wpos + ImVec2{ 0.0f, CurrentY }, rs.wpos + ImVec2{ rs.wsize.x, CurrentY + 20.0f }, IM_COL32(90, 90, 90, 255));
    ImGui::GetWindowDrawList()->AddRect(rs.wpos + ImVec2{ 0.0f, CurrentY }, rs.wpos + ImVec2{ rs.wsize.x, CurrentY + 20.0f }, SelectedAgent == agent.Name ? IM_COL32(40, 40, 140, 255) : IM_COL32(10, 10, 10, 255));
    ImGui::SetCursorScreenPos(rs.wpos + ImVec2{ 18.0f, CurrentY + 4.0f });
    ImGui::PushFont(ImGui::GetFont(), 12.0f);
    ImGui::TextUnformatted(agent.Name.c_str());
    ImGui::PopFont();
    if (EditorInstance->ImageButton(open ? "Chore/AgentOpen.png" : "Chore/AgentUnopen.png", 0.0f, CurrentY + 1.0f, 18.0f, 18.0f))
    {
        if (open)
        {
            OpenChoreAgents.erase(OpenChoreAgents.find(agent.Name));
        }
        else
        {
            OpenChoreAgents.insert(agent.Name);
        }
        open = !open;
    }
    if (leftClicked && ImGui::IsMouseHoveringRect(rs.wpos + ImVec2{ 0.0f, CurrentY }, rs.wpos + ImVec2{ rs.wsize.x, CurrentY + 20.0f }, false))
    {
        SelectedResourceIndex = -1;
        SelectedAgent = agent.Name;
        rs.anythingClicked = true;
    }

    // unless we have dynamic menu options
    /*ImGui::PushID(agent.Name.c_str());
    if(SelectedAgent == agent.Name)
        OpenContextMenu(agent.Name.c_str(), wpos + ImVec2{ 0.0f, CurrentY }, wpos + ImVec2{ wsize.x, CurrentY + 20.0f });
    ImGui::PopID();*/

    CurrentY += 20.f;
    if (open)
    {
        ImGui::GetWindowDrawList()->AddRectFilled(rs.wpos + ImVec2{ 0.0f, CurrentY }, rs.wpos + ImVec2{ rs.wsize.x, CurrentY + 5.0f }, IM_COL32(205, 212, 201, 255));
        CurrentY += 2.0f;
        _RenderResourceState state{rs};
        state.leftClicked = leftClicked;
        state.runningID = 412433;
        state.rmResource = -1;
        state.resourceIndex = 0;
        for (const auto& resIndex : agent.Resources)
        {
            ImGui::PushID(state.runningID++);
            Chore::Resource& resource = EditorInstance->GetCommonObject()->_Resources[resIndex];
            state.agentResourceIndex = resIndex;
            RenderChoreResource(pChore, resource, agent, state);
            ImGui::PopID();
            state.resourceIndex++;
        }
        if (state.rmResource != -1)
        {
            I32 concreteResourceIndex = agent.Resources[state.rmResource];
            agent.Resources.erase(agent.Resources.begin() + state.rmResource);
            Bool bHasRef = false;
            for (auto rit = pChore->_Agents.begin(); rit != pChore->_Agents.end(); rit++)
            {
                for (const auto& resIndex : rit->Resources)
                {
                    if (resIndex == concreteResourceIndex)
                    {
                        bHasRef = true;
                        break;
                    }
                }
                if (bHasRef)
                    break;
            }
            if (!bHasRef)
            {
                // remove actual resource
                pChore->_DoRemoveResource(concreteResourceIndex);
            }
        }
    }
}

void UIResourceEditorRuntimeData<Chore>::RenderChoreTimeline(const Ptr<Chore>& pChore, const Float& RES_HEIGHT, const ImVec2& wpos, const ImVec2& wsize, Bool leftClicked)
{
    CurrentY -= ImGui::GetScrollY();
    _RenderState rs{ RES_HEIGHT, wpos, wsize };
    Float cache = CurrentY;
    rs.mouseClickedThisFrame = leftClicked && GImGui->OpenPopupStack.empty() && ImGui::IsWindowFocused();
    rs.mouseReleasedThisFrame = ImGui::IsMouseReleased(ImGuiMouseButton_Left) && GImGui->OpenPopupStack.empty() && ImGui::IsWindowFocused();
    rs.mouseRightReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Right) && GImGui->OpenPopupStack.empty() && ImGui::IsWindowFocused();
    rs.mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    rs.mouseDeltaX = ImGui::GetMousePos().x - LastMouseX;
    rs.mouseDeltaY = ImGui::GetMousePos().y - LastMouseY;
    rs.anySamplesClicked = false;
    rs.anythingClicked = false;
    rs.selectionRectMin = { SelectionBox1X, SelectionBox1Y };
    rs.selectionRectMax = { SelectionBox2X, SelectionBox2Y };
    rs.usedSelectionReclick = false;
    rs.allowReclickNextFrame = false;

    if (rs.mouseReleasedThisFrame && ScaleMode)
    {
        ActiveScaleMode = NONE;
    }

    // Render each agent
    I32 agentID = 100;
    for (auto& agent : EditorInstance->GetCommonObject()->_Agents)
    {
        ImGui::PushID(agentID++);
        RenderChoreAgent(pChore, agent, rs, leftClicked);
        ImGui::PopID();
    } // END PER AGENT BLOCK

    // SELECTION BOX UTIL

    SelectionBoxReady = false;
    if (!rs.anythingClicked && rs.mouseClickedThisFrame)
    {
        if (SelectionBoxReclickAvail && !rs.usedSelectionReclick)
        {
            rs.usedSelectionReclick = true; // reselect in selection box
        }
        else
        {
            SelectedKeyframeSamples.clear();
            SelectedResourceBlocks.clear();
            SelectionBox1X = SelectionBox2X = ImGui::GetMousePos().x;
            SelectionBox1Y = SelectionBox2Y = ImGui::GetMousePos().y;
            SelectionBoxDragging = true;
        }
    }
    if (SelectionBoxDragging)
    {
        SelectionBox2X = ImGui::GetMousePos().x;
        SelectionBox2Y = ImGui::GetMousePos().y;
        ImGui::GetWindowDrawList()->AddRect(ImVec2{ MIN(SelectionBox1X, SelectionBox2X), MIN(SelectionBox1Y, SelectionBox2Y) },
            ImVec2{ MAX(SelectionBox1X, SelectionBox2X), MAX(SelectionBox1Y, SelectionBox2Y) }, IM_COL32(80, 80, 80, 255), 2.0f, 0, 3.0f);
        if (rs.mouseReleasedThisFrame)
        {
            ImVec2 mn = ImMin(ImVec2{ SelectionBox1X, SelectionBox1Y }, ImVec2{ SelectionBox2X, SelectionBox2Y });
            ImVec2 mx = ImMax(ImVec2{ SelectionBox1X, SelectionBox1Y }, ImVec2{ SelectionBox2X, SelectionBox2Y });
            SelectionBox1X = mn.x; SelectionBox1Y = mn.y;
            SelectionBox2X = mx.x; SelectionBox2Y = mx.y;
            SelectionBoxDragging = false;
            SelectionBoxReady = true;
            SelectionBoxReclickAvail = true;
        }
    }
    if (rs.usedSelectionReclick)
    {
        SelectionBoxReclickAvail = false;
    }
    if (rs.allowReclickNextFrame)
    {
        SelectionBoxReclickAvail = true;
    }
    Float dy = CurrentY - cache;
    ImGui::SetCursorScreenPos(wpos + ImVec2{ 0.0f, cache });
    ImGui::Dummy(ImVec2{ 1.0f, dy });
    ImGui::SetCursorScreenPos(wpos);
    ImGui::GetWindowDrawList()->ChannelsMerge();
    LastMouseX = ImGui::GetMousePos().x;
    LastMouseY = ImGui::GetMousePos().y;
}

void UIResourceEditorRuntimeData<Chore>::RenderMenuOptions(Bool& closing, const Ptr<Chore>& pChore)
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Close"))
            {
                closing = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            AddMenuOptions("Edit");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            AddMenuOptions("View");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Agent"))
        {
            if (ImGui::BeginMenu("Include an agent"))
            {
                if (ImGui::MenuItem("By Name"))
                {
                    EditorInstance->GetApplication().QueueMetaInstanceEditPopup(EditorInstance->_EditorUI, "New Agent for Chore",
                        ALLOCATE_METHOD_CALLBACK_1(this, AddAgentCallback, UIResourceEditorRuntimeData<Chore>, Meta::ClassInstance),
                        "Agent Name", Meta::CreateInstance(Meta::FindClass("String", 0)));
                }
                if (!EditorInstance->_EditorUI.GetActiveScene().GetName().empty() && ImGui::BeginMenu(EditorInstance->_EditorUI.GetActiveScene().GetName().c_str()))
                {
                    for (const auto& agent : EditorInstance->_EditorUI.GetActiveScene().GetAgents())
                    {
                        String agentName = EditorInstance->_EditorUI.GetActiveScene().GetAgentNameString(agent.first);
                        if (ImGui::MenuItem(agentName.c_str()))
                        {
                            DoAddAgent(agentName);
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        /*if(ImGui::BeginMenu("Resources"))
        {
            AddMenuOptions("Resources");
            ImGui::EndMenu();
        }*/
        /*if(ImGui::BeginMenu("Behaviour"))
        {
            ImGui::EndMenu();
        }*/
        if (!SelectedAgent.empty() && ImGui::BeginMenu(SelectedAgent.c_str()))
        {
            if (ImGui::MenuItem("Add Animation"))
            {
                EditorInstance->GetApplication().QueueResourcePickerPopup(EditorInstance->_EditorUI, "Choose Animation", "*.anm",
                    ALLOCATE_METHOD_CALLBACK_1(this, AddAgentResourceCallback, UIResourceEditorRuntimeData<Chore>, String));
            }
            if (ImGui::MenuItem("Add Chore"))
            {
                EditorInstance->GetApplication().QueueResourcePickerPopup(EditorInstance->_EditorUI, "Choose Chore", "*.chore",
                    ALLOCATE_METHOD_CALLBACK_1(this, AddAgentResourceCallback, UIResourceEditorRuntimeData<Chore>, String));
            }
            // AUD, VOX, LANGUAGE RESOURCES
            if (ImGui::BeginMenu("Add Procedural Animation"))
            {
                if (ImGui::MenuItem("Look At"))
                {
                    Bool exist = false;
                    // Ill keep these here in case. but you can have multiple look ats (obviously, might want to lookat different characters at different times from this same one)
                    /*const Chore::Agent* agent = pChore->GetAgent(SelectedAgent);
                    for(const auto& res: agent->Resources)
                    {
                        auto pRes = pChore->GetConcreteResource(agent->Name, pChore->GetResources()[res].Name);
                        if(pRes && std::dynamic_pointer_cast<Procedural_LookAt>(pRes))
                        {
                            exist = true;
                            PlatformMessageBoxAndWait("Error", GetApplication().GetLanguageText("misc.procedural_error"));
                            break;
                        }
                    }*/
                    if (!exist)
                    {
                        DoAddAgentResourcePostLoadCallback("look");
                    }
                }
                // TODO EYES
                ImGui::EndMenu();
            }
            // blocking anims for moving agents around manually
            if (ImGui::MenuItem("Add Blocking Animation"))
            {

            }
            // OTHER RESOURCE
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    if (TestMenuOption("View", "Clear Selection", "[ESC]", ImGuiKey_Escape, false, false))
    {
        SelectedKeyframeSamples.clear();
        SelectedResourceBlocks.clear();
        SelectedResourceIndex = -1;
    }
    if (TestMenuOption("View", "Tidy", "[CTRL + T]", ImGuiKey_T, false, true))
    {
        // close all graphs and properties
        for (auto& resource : pChore->_Resources)
        {
            resource.ResFlags.Remove(Chore::Resource::VIEW_GRAPHS);
            resource.ResFlags.Remove(Chore::Resource::VIEW_PROPERTIES);
            resource.ResFlags.Remove(Chore::Resource::VIEW_GROUPS);
        }
    }
    if (TestMenuOption("View", "Untidy", "[SHIFT + CTRL + T]", ImGuiKey_T, true, true))
    {
        // open all graphs and properties
        for (auto& resource : pChore->_Resources)
        {
            resource.ResFlags.Add(Chore::Resource::VIEW_GRAPHS);
            resource.ResFlags.Add(Chore::Resource::VIEW_PROPERTIES);
            resource.ResFlags.Add(Chore::Resource::VIEW_GROUPS);
        }
    }
    if (TestMenuOption("View", "Collapse All", "", 0, false, false))
    {
        OpenChoreAgents.clear();
    }
}

void UIResourceEditorRuntimeData<Chore>::RenderChoreGraphKeyframedFloat(_RenderState& rs, _RenderGraphState& gstate, Bool bAddNew)
{
    Ptr<KeyframedValue<Float>> fkf = std::dynamic_pointer_cast<KeyframedValue<Float>>(gstate.pAnimatedValue);
    TTE_ASSERT(fkf, ""); // should be nonnull already

    if (bAddNew)
    {
        Float valueAt{ 1.0f };
        Flags _{};
        fkf->ComputeValueKeyframed(&valueAt, CurrentTime, kDefaultContribution, _, true);
        fkf->InsertSample(CurrentTime, valueAt);
    }

    // LINES FIRST
    const Float timelineWidth = gstate.resourceBoxMax.x - gstate.resourceBoxMin.x;
    Float prevSampleX = 0.0f, prevSampleY = 0.0f;
    for (I32 i = 0; i < fkf->GetSamples().size(); i++)
    {
        auto& sample = fkf->GetSamples()[i];
        Float sampleX = TO_SCREENSPACE_XXX(sample.Time, gstate.resourceBoxMin);
        Float sampleY = gstate.resourceBoxMin.y + ImClamp(1.0f - sample.Value, 0.0f, 1.0f) * (gstate.resourceBoxMax.y - gstate.resourceBoxMin.y);
        if (i > 0)
        {
            ImGui::GetWindowDrawList()->AddLine(ImVec2{ prevSampleX, prevSampleY }, ImVec2{ sampleX, sampleY }, IM_COL32(0, 0, 0, 255));
        }
        prevSampleX = sampleX; prevSampleY = sampleY;
    }
    // GRAB HANDLES
    for (I32 i = 0; i < fkf->GetSamples().size(); i++)
    {
        auto& sample = fkf->GetSamples()[i];
        Float sampleX = TO_SCREENSPACE_XXX(sample.Time, gstate.resourceBoxMin);
        Float sampleY = gstate.resourceBoxMin.y + ImClamp(1.0f - sample.Value, 0.0f, 1.0f) * (gstate.resourceBoxMax.y - gstate.resourceBoxMin.y);
        ImVec2 sampleGrabMin = ImVec2{ sampleX - 3.0f, sampleY - 2.0f };
        ImVec2 sampleGrabMax = ImVec2{ sampleX + 3.0f, sampleY + 2.0f };
        if (SelectionBoxReady && ImRect{ rs.selectionRectMin, rs.selectionRectMax }.Overlaps(ImRect{ sampleGrabMin, sampleGrabMax }))
            SelectedKeyframeSamples.insert(&sample);
        if (ImGui::IsMouseHoveringRect(sampleGrabMin - ImVec2{ 3.0f, 3.0f }, sampleGrabMax + ImVec2{ 3.0f, 3.0f }, false) && rs.mouseClickedThisFrame)
        {
            if (SelectionBoxReclickAvail && !rs.usedSelectionReclick)
            {
                rs.usedSelectionReclick = true; // reselect in selection box
            }
            else
            {
                if (!ImGui::IsKeyDown(ImGuiKey_LeftShift))
                {
                    SelectedKeyframeSamples.clear();
                }
                SelectedKeyframeSamples.insert(&sample);
            }
            rs.anySamplesClicked = true;
            rs.anythingClicked = true;
        }
        Bool bImSelec = SelectedKeyframeSamples.find(&sample) != SelectedKeyframeSamples.end();
        ImGui::GetWindowDrawList()->AddRectFilled(sampleGrabMin, sampleGrabMax, bImSelec ? IM_COL32(12, 105, 13, 255) : IM_COL32(8, 69, 9, 255));
        if (bImSelec && rs.mouseDown)
        {
            Float deltaTime = (rs.mouseDeltaX / timelineWidth) * (ViewEnd - ViewStart);
            // into 0-1 space
            Float deltaValue = -(rs.mouseDeltaY / (gstate.resourceBoxMax.y - gstate.resourceBoxMin.y));
            Float newTime = sample.Time + deltaTime;
            Float newValue = sample.Value + deltaValue;
            if (i > 0)
                newTime = ImMax(newTime, fkf->GetSamples()[i - 1].Time + 0.0001f);
            if (i < fkf->GetSamples().size() - 1)
                newTime = ImMin(newTime, fkf->GetSamples()[i + 1].Time - 0.0001f);
            newTime = ImClamp(newTime, 0.0f, EditorInstance->GetCommonObject()->GetLength());
            newValue = ImClamp(newValue, 0.0f, 1.0f);
            if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
            {
                // only do one axis, (with most movement)
                if (fabsf(rs.mouseDeltaX) > fabsf(rs.mouseDeltaY))
                    sample.Time = newTime;
                else
                    sample.Value = newValue;
            }
            else
            {
                sample.Time = newTime;
                sample.Value = newValue;
            }

            //Float newSampleX = ImClamp(FROM_SCREENSPACE(ImGui::GetMousePos().x),
            //    i == 0 ? 0.0f : fkf->GetSamples()[i - 1].Time + 0.0001f, i == fkf->GetSamples().size() - 1 ? pChore->GetLength() : fkf->GetSamples()[i + 1].Time);
            //Float newSampleY = ImClamp(((ImGui::GetMousePos().y - resourceBoxMin.y) / (resourceBoxMax.y - resourceBoxMin.y)), 0.0f, 1.0f);
            //sample.Time = newSampleX;
            //sample.Value = 1.0f - newSampleY;
        }
    }
    if (fkf->GetSamples().size() > 1)
    {
        ImGui::GetWindowDrawList()->AddLine(ImVec2{ TO_SCREENSPACE_XXX(fkf->GetSamples()[0].Time, gstate.resourceBoxMin), gstate.resourceBoxMin.y - 2.0f },
            ImVec2{ TO_SCREENSPACE_XXX(fkf->GetSamples()[0].Time, gstate.resourceBoxMin), gstate.resourceBoxMax.y + 2.0f }, IM_COL32(0, 0, 0, 255));
        ImGui::GetWindowDrawList()->AddLine(ImVec2{ TO_SCREENSPACE_XXX(fkf->GetMaxTime(), gstate.resourceBoxMin), gstate.resourceBoxMin.y - 2.0f },
            ImVec2{ TO_SCREENSPACE_XXX(fkf->GetMaxTime(), gstate.resourceBoxMin), gstate.resourceBoxMax.y + 2.0f }, IM_COL32(0, 0, 0, 255));
    }
}

void UIResourceEditorRuntimeData<Chore>::RenderChoreGraphKeyframedBool(_RenderState& rs, _RenderGraphState& gstate, Bool bAddNew)
{
    const Float CENTER_LINE_THICKNESS = 8.0f;
    const ImU32 BOOL_COLOUR[2] = { IM_COL32(120, 10, 10, 255), IM_COL32(10, 180, 10, 255) };
    Ptr<KeyframedValue<Bool>> kfv = std::dynamic_pointer_cast<KeyframedValue<Bool>>(gstate.pAnimatedValue);
    TTE_ASSERT(kfv, ""); // should be nonnull already

    if(bAddNew)
    {
        Bool False = false; // default to false
        kfv->InsertSampleAt(CurrentTime, &False);
    }

    const Float timelineWidth = gstate.resourceBoxMax.x - gstate.resourceBoxMin.x;
    Float prevSampleX = 0.0f, prevSampleY = 0.0f;
    for (I32 i = 0; i < kfv->GetSamples().size(); i++)
    {
        auto& sample = kfv->GetSamples()[i];
        Float sampleX = TO_SCREENSPACE_XXX(sample.Time, gstate.resourceBoxMin);
        Float sampleY = (gstate.resourceBoxMin.y + gstate.resourceBoxMax.y - CENTER_LINE_THICKNESS) * 0.5f;
        if (i > 0)
        {
            ImGui::GetWindowDrawList()->AddLine(ImVec2{ prevSampleX, sampleY }, ImVec2{ sampleX, sampleY + CENTER_LINE_THICKNESS }, BOOL_COLOUR[sample.Value?1:0]);
        }
        prevSampleX = sampleX; prevSampleY = sampleY;
    }
}

void UIResourceEditorRuntimeData<Chore>::RenderChoreGraphKeyframedOther(_RenderState& rs, _RenderGraphState& gstate, Bool bAddNew)
{
    Ptr<KeyframedValueInterface> kfv = std::dynamic_pointer_cast<KeyframedValueInterface>(gstate.pAnimatedValue);
    TTE_ASSERT(kfv, ""); // should be nonnull already

    if (bAddNew)
    {
        kfv->InsertSampleAt(CurrentTime, nullptr);
    }
}

void UIResourceEditorRuntimeData<Chore>::RenderChoreGraph(Chore::Resource& resource, UIResourceEditor<Chore>& editor, _RenderState& rs, const Ptr<AnimationValueInterface>& pAnimatedValue)
{
    Bool bAddNew = false;
    Ptr<KeyframedValueInterface> kfv = std::dynamic_pointer_cast<KeyframedValueInterface>(pAnimatedValue);

    // =========================================================================================================================================================================
    // ========================================================================= ROW OUTLINE BOX & ICO =========================================================================
    // =========================================================================================================================================================================

    ImGui::GetWindowDrawList()->AddRectFilled(rs.wpos + ImVec2{ 0.0f, CurrentY }, rs.wpos + ImVec2{ rs.wsize.x, CurrentY + rs.RES_HEIGHT + 2.0f }, IM_COL32(205, 212, 201, 255));
    ImVec2 resourceBoxMin = rs.wpos + ImVec2{ 12.0f, CurrentY };
    ImVec2 resourceBoxMax = rs.wpos + ImVec2{ rs.wsize.x - 6.0f, CurrentY + rs.RES_HEIGHT };
    ImGui::GetWindowDrawList()->AddRectFilled(resourceBoxMin, resourceBoxMax, IM_COL32(206, 222, 173, 255));
    ImGui::GetWindowDrawList()->AddRect(resourceBoxMin, resourceBoxMax, IM_COL32(20, 20, 20, 255));
    ImGui::GetWindowDrawList()->PushClipRect(resourceBoxMin, resourceBoxMax, false);
    if (ImGui::IsMouseHoveringRect(resourceBoxMin, resourceBoxMax, false) && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        OpenContextMenuGraph = pAnimatedValue.get();
    }
    Bool bLastClicked = OpenContextMenuGraph == pAnimatedValue.get();
    resourceBoxMin.x += 3.0f;
    resourceBoxMax.x -= 3.0f;
    resourceBoxMin.y += 2.0f;
    resourceBoxMax.y -= 2.0f;
    ImGui::PushFont(ImGui::GetFont(), 12.0f);
    ImVec2 tSize = ImGui::CalcTextSize(pAnimatedValue->GetName().c_str());
    editor.DrawResourceTexturePixels("Chore/Graph.png", rs.wsize.x - 30.0f, CurrentY + 5.0f, 21.0f, 21.0f);
    ImGui::SetCursorScreenPos(rs.wpos + ImVec2{ rs.wsize.x - tSize.x - 33.f, CurrentY + 16.0f });
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
    ImGui::TextUnformatted(pAnimatedValue->GetName().c_str());
    ImGui::PopStyleColor();
    ImGui::PopFont();

    // =========================================================================================================================================================================
    // ========================================================================= GRAPH RIGHT CLICK OPT =========================================================================
    // =========================================================================================================================================================================

    if (OpenContextMenu("graph", resourceBoxMin, resourceBoxMax))
        rs.allowReclickNextFrame = true; // graph options check

    if (kfv) // only allow if a keyframed value for now.
    {

        // MENU OPTIONS:
        // EDIT SELECTED KEYS, EDIT ALL KEYS (FOR THIS ANIMATED) ??? (update sep bool if adding)
        // STEPPED, LINEAR, FLAT, SMOOTH, CREATE GUIDE
        // // FIT RANGE,SET RANGE (these seem to be for somethjing else)

        if (bLastClicked && TestMenuOption("graph", "Disable", "", 0, false, false, false, pAnimatedValue->GetFlags().Test(AnimationValueFlags::DISABLED) ? "Enable" : nullptr))
        {
            pAnimatedValue->SetDisabled(!pAnimatedValue->GetFlags().Test(AnimationValueFlags::DISABLED));
        }
        if (bLastClicked && TestMenuOption("graph", "Add New Key", "[K]", ImGuiKey_K, false, false))
        {
            SelectedKeyframeSamples.clear();
            bAddNew = true;
        }
        if (bLastClicked && TestMenuOption("graph", "Delete Selected Keys", "[BACKSPACE]", ImGuiKey_Backspace, false, false, true))
        {
            std::vector<I32> deleteIndices;

            for (auto ptr : SelectedKeyframeSamples)
            {
                for (I32 i = 0; i < kfv->GetNumSamples(); i++)
                {
                    if (kfv->GetSample(i) == ptr)
                    {
                        deleteIndices.push_back(i);
                        break;
                    }
                }
            }
            std::sort(deleteIndices.rbegin(), deleteIndices.rend());
            for (I32 idx : deleteIndices)
            {
                kfv->RemoveSample(idx);
            }
            SelectedKeyframeSamples.clear();
        }
    }

    // editable with drags if its a float, else individual keys, else not avail if not a kfv
    Ptr<KeyframedValue<Float>> fkf = std::dynamic_pointer_cast<KeyframedValue<Float>>(pAnimatedValue);
    _RenderGraphState gstate{ pAnimatedValue, resourceBoxMin, resourceBoxMax };
    if(fkf)
    {
        RenderChoreGraphKeyframedFloat(rs, gstate, bAddNew);
    }
    else if (std::dynamic_pointer_cast<KeyframedValue<Bool>>(pAnimatedValue))
    {
        RenderChoreGraphKeyframedBool(rs, gstate, bAddNew);
    }
    else if(kfv)
    {
        RenderChoreGraphKeyframedOther(rs, gstate, bAddNew);
    }
    else
    {
        ImGui::SetCursorScreenPos(resourceBoxMin);
        ImGui::Text("UNK_GRAPH:%s", pAnimatedValue->GetValueType().name());
    }

    ImGui::PopClipRect();
    CurrentY += rs.RES_HEIGHT + 2.0f;
}

void UIResourceEditorRuntimeData<Chore>::RenderChoreGraphs(Chore::Resource& resource, UIResourceEditor<Chore>& editor, _RenderState& rs, I32& runningID)
{
    if (resource.ResFlags.Test(Chore::Resource::VIEW_GRAPHS))
    {
        // GRAPHS FOR CONTROL ANIMATION
        for (auto& animatedValue : resource.ControlAnimation->GetAnimatedValues())
        {
            if (animatedValue->GetName() != "contribution" && animatedValue->GetName() != "time")
                continue; // control animation just plays throughout the chore, acting on the agent in the scene.
            ImGui::PushID(runningID++);
            RenderChoreGraph(resource, editor, rs, animatedValue);
            ImGui::PopID();
        }
    }
}