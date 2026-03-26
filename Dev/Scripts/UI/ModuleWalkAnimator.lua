function ModuleWalkAnimator_RegisterUI(moduleVersion) -- version 0 for anims, ver 1 for anim or chores

    local inputType = moduleVersion > 0 and kPropRenderAnimOrChore or "handle:anm"
    local subPath = moduleVersion > 0 and "this" or "this.mHandle"

    local dataTable = {}
    dataTable["Forward Animation"] = {Class = "class Handle<class Animation>", Key = kWalkAnimatorForwardAnimation}
    dataTable["Forward Animation"]["UI"] = {InputType = inputType, SubPath = subPath}

    dataTable["Idle Animation"] = {Class = "class Handle<class Animation>", Key = kWalkAnimatorIdleAnimation}
    dataTable["Idle Animation"]["UI"] = {InputType = inputType, SubPath = subPath}

    RegisterModuleUI("walkAnimator", "Module/WalkAnimator.png", dataTable)

end