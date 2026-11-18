                                                                                            
                                                        
  
                        
                                                                                          
                                                                              
                                                                 
                                                                                         
                                                                                              
                                                                                 
                                                                    
                                                                                           
                                                                                     
                                                                                                
                                                                 
                                                                                               
#import "PwLodSurface.h"

#import <Foundation/Foundation.h>
#import <IOSurface/IOSurfaceRef.h>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct PwLodSurfaceRing {
  uint32_t width;
  uint32_t height;
  IOSurfaceRef surfaces[PWLOD_TARGET_COUNT];
  CVPixelBufferRef pixel_buffers[PWLOD_TARGET_COUNT];
  pwlod_target targets[PWLOD_TARGET_COUNT];
};

static void PwLodSurfaceError(char *buf, uint32_t len, const char *fmt, ...) {
  if (buf == NULL || len == 0) return;
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, len, fmt, ap);
  va_end(ap);
}

pwlod_status PwLodSurfaceCreateGpu(pwlod_gpu *out_gpu) {
                                                                                  
                                                                          
                                                                           
                                                                            
                                                                          
  const WGPUFeatureName features[] = {
      WGPUFeatureName_SharedTextureMemoryIOSurface,
      WGPUFeatureName_SharedFenceMTLSharedEvent,
      WGPUFeatureName_ImplicitDeviceSynchronization,
  };
  return pwlod_gpu_create(features,
      (uint32_t)(sizeof(features) / sizeof(features[0])), out_gpu);
}

WGPUTextureFormat PwLodSurfaceRingFormat(void) {
                                             
                                                                            
  return WGPUTextureFormat_BGRA8Unorm;
}

const pwlod_target *PwLodSurfaceRingTargets(const PwLodSurfaceRing *ring) {
  return ring != NULL ? ring->targets : NULL;
}

CVPixelBufferRef PwLodSurfaceRingPixelBuffer(const PwLodSurfaceRing *ring, uint32_t index) {
  if (ring == NULL || index >= PWLOD_TARGET_COUNT) return NULL;
  return ring->pixel_buffers[index];
}

void PwLodSurfaceRingDestroy(PwLodSurfaceRing *ring) {
  if (ring == NULL) return;
  for (uint32_t i = 0; i < PWLOD_TARGET_COUNT; ++i) {
    if (ring->targets[i].texture != NULL) wgpuTextureRelease(ring->targets[i].texture);
    if (ring->targets[i].memory != NULL) wgpuSharedTextureMemoryRelease(ring->targets[i].memory);
    if (ring->pixel_buffers[i] != NULL) CVPixelBufferRelease(ring->pixel_buffers[i]);
    if (ring->surfaces[i] != NULL) CFRelease(ring->surfaces[i]);
  }
  free(ring);
}

                                                                        
static int PwLodSurfaceImport(WGPUDevice device,
                              IOSurfaceRef surface,
                              uint32_t width,
                              uint32_t height,
                              pwlod_target *out,
                              char *err,
                              uint32_t err_len) {
  WGPUSharedTextureMemoryIOSurfaceDescriptor io_desc =
      WGPU_SHARED_TEXTURE_MEMORY_IO_SURFACE_DESCRIPTOR_INIT;
  io_desc.ioSurface = (void *)surface;
                                                                                               
  io_desc.allowStorageBinding = WGPU_FALSE;

  WGPUSharedTextureMemoryDescriptor mem_desc = WGPU_SHARED_TEXTURE_MEMORY_DESCRIPTOR_INIT;
  mem_desc.nextInChain = &io_desc.chain;

  WGPUSharedTextureMemory mem = wgpuDeviceImportSharedTextureMemory(device, &mem_desc);
  if (mem == NULL) {
    PwLodSurfaceError(err, err_len,
                      "ImportSharedTextureMemory NULL (feature not granted / IOSurface format?)");
    return 0;
  }

  WGPUSharedTextureMemoryProperties props = WGPU_SHARED_TEXTURE_MEMORY_PROPERTIES_INIT;
  const WGPUStatus props_status = wgpuSharedTextureMemoryGetProperties(mem, &props);
  if (props_status != WGPUStatus_Success) {
    PwLodSurfaceError(err, err_len, "SharedTextureMemoryGetProperties status=%d",
                      (int)props_status);
    wgpuSharedTextureMemoryRelease(mem);
    return 0;
  }
                                                                                          
                                                                                             
                                                                                         
                               
  if (props.size.width == 0 || props.size.height == 0) {
    PwLodSurfaceError(err, err_len,
                      "ImportSharedTextureMemory returned an error object (0x0): feature not "
                      "granted or IOSurface rejected -- see Dawn's uncaptured-error log");
    wgpuSharedTextureMemoryRelease(mem);
    return 0;
  }
  if (props.size.width != width || props.size.height != height) {
    PwLodSurfaceError(err, err_len, "size mismatch sharedMemory=(%u,%u) requested=(%u,%u)",
                      props.size.width, props.size.height, width, height);
    wgpuSharedTextureMemoryRelease(mem);
    return 0;
  }
  if (props.format != PwLodSurfaceRingFormat()) {
    PwLodSurfaceError(err, err_len, "format mismatch sharedMemory=%d requested=%d",
                      (int)props.format, (int)PwLodSurfaceRingFormat());
    wgpuSharedTextureMemoryRelease(mem);
    return 0;
  }
  const WGPUTextureUsage needed = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
  if ((props.usage & needed) != needed) {
    PwLodSurfaceError(err, err_len, "usage 0x%llx lacks RenderAttachment|CopySrc",
                      (unsigned long long)props.usage);
    wgpuSharedTextureMemoryRelease(mem);
    return 0;
  }

  WGPUTextureDescriptor texture_desc = WGPU_TEXTURE_DESCRIPTOR_INIT;
  texture_desc.usage = props.usage;
  texture_desc.dimension = WGPUTextureDimension_2D;
  texture_desc.size = props.size;
  texture_desc.format = props.format;
  texture_desc.mipLevelCount = 1;
  texture_desc.sampleCount = 1;

  WGPUTexture texture = wgpuSharedTextureMemoryCreateTexture(mem, &texture_desc);
  if (texture == NULL) {
    PwLodSurfaceError(err, err_len, "SharedTextureMemoryCreateTexture NULL");
    wgpuSharedTextureMemoryRelease(mem);
    return 0;
  }
  out->texture = texture;
  out->memory = mem;
  return 1;
}

PwLodSurfaceRing *PwLodSurfaceRingCreate(WGPUDevice device,
                                         uint32_t width,
                                         uint32_t height,
                                         char *err_buf,
                                         uint32_t err_buf_len) {
  if (device == NULL || width == 0 || height == 0) {
    PwLodSurfaceError(err_buf, err_buf_len, "invalid args device=%p %ux%u", (void *)device,
                      width, height);
    return NULL;
  }
  PwLodSurfaceRing *ring = (PwLodSurfaceRing *)calloc(1, sizeof(PwLodSurfaceRing));
  if (ring == NULL) {
    PwLodSurfaceError(err_buf, err_buf_len, "calloc failed");
    return NULL;
  }
  ring->width = width;
  ring->height = height;

  const size_t bytes_per_row = IOSurfaceAlignProperty(kIOSurfaceBytesPerRow, (size_t)width * 4);
                                                       
  NSDictionary *props = @{
    (__bridge NSString *)kIOSurfaceWidth : @(width),
    (__bridge NSString *)kIOSurfaceHeight : @(height),
    (__bridge NSString *)kIOSurfacePixelFormat : @(kCVPixelFormatType_32BGRA),
    (__bridge NSString *)kIOSurfaceBytesPerElement : @4,
    (__bridge NSString *)kIOSurfaceBytesPerRow : @(bytes_per_row),
  };

  for (uint32_t i = 0; i < PWLOD_TARGET_COUNT; ++i) {
    IOSurfaceRef surface = IOSurfaceCreate((__bridge CFDictionaryRef)props);
    if (surface == NULL) {
      PwLodSurfaceError(err_buf, err_buf_len, "IOSurfaceCreate NULL (slot %u, %ux%u)", i, width,
                        height);
      PwLodSurfaceRingDestroy(ring);
      return NULL;
    }
    ring->surfaces[i] = surface;

    CVPixelBufferRef pb = NULL;
    const CVReturn cv =
        CVPixelBufferCreateWithIOSurface(kCFAllocatorDefault, surface, NULL, &pb);
    if (cv != kCVReturnSuccess || pb == NULL) {
      PwLodSurfaceError(err_buf, err_buf_len, "CVPixelBufferCreateWithIOSurface CVReturn=%d",
                        (int)cv);
      PwLodSurfaceRingDestroy(ring);
      return NULL;
    }
    ring->pixel_buffers[i] = pb;

    char slot_err[160] = {0};
    if (!PwLodSurfaceImport(device, surface, width, height, &ring->targets[i], slot_err,
                            sizeof slot_err)) {
      PwLodSurfaceError(err_buf, err_buf_len, "slot %u: %s", i, slot_err);
      PwLodSurfaceRingDestroy(ring);
      return NULL;
    }
  }
  return ring;
}
