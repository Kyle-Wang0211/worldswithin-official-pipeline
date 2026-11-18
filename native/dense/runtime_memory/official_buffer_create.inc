  if(!pw_upload::request.armed) {
  bool official_fresh=false;
  const auto official=reconstruction_official_object::pool().acquire(uintptr_t(context_.Device().Get()),uintptr_t(this),size,uint64_t(usage),
    [&](size_t capacity)->uint64_t{wgpu::BufferDescriptor d{};d.size=capacity;d.usage=usage;return uintptr_t(S1BudgetCreate(context_.Device(),d).MoveToCHandle());},
    +[](uint64_t h){wgpuBufferAddRef(reinterpret_cast<WGPUBuffer>(h));},
    +[](uint64_t h){wgpuBufferRelease(reinterpret_cast<WGPUBuffer>(h));},&official_fresh);
  if(official){auto buffer=reinterpret_cast<WGPUBuffer>(official);S1MemoryEvent(context_.Device().Get(),0,3);return buffer;}
  }
