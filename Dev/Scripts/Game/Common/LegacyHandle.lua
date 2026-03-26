-- Old Handle<T> types use a string, but have a custom serialiser which disables the blocked string. We implement this here

function SerialiseLegacyHandle(stream, instance, isWrite)
    if isWrite then 
        MetaStreamWriteString(stream, MetaGetClassValue(MetaGetMember(instance, "mHandle"))) 
    else
        MetaSetClassValue(MetaGetMember(instance, "mHandle"), MetaStreamReadString(stream))
    end
    return true
end

function RegisterLegacyHandle(name)
    local MetaHandle = { VersionIndex = 0 }
    MetaHandle.Name = name
    MetaHandle.Flags = kMetaClassIntrinsic
    MetaHandle.Serialiser = "SerialiseLegacyHandle"
    MetaHandle.Members = {}
    MetaHandle.Members[1] = { Name = "mHandle", Class = kMetaClassString, Flags = kMetaMemberVersionDisable }
    MetaRegisterClass(MetaHandle)
    return MetaHandle
end