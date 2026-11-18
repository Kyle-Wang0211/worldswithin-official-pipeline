                                                                           
  
                                                                                                
                                                                                         
                                                                                              
                                                                 
  
                                                                                               
                                                                                        
                                                                                     
                                      
  
                                                                             
                                                                                  

#include <VideoToolbox/VideoToolbox.h>
#include <CoreMedia/CoreMedia.h>
#include <CoreVideo/CoreVideo.h>
#include <dispatch/dispatch.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t *data;
    size_t len;
    int32_t is_keyframe;
    OSStatus status;
} PwOutput;

typedef struct PwVtEncoder {
    VTCompressionSessionRef session;
    dispatch_semaphore_t done;
    PwOutput pending;
    int32_t width;
    int32_t height;
} PwVtEncoder;

static void append_bytes(uint8_t **buf, size_t *len, size_t *cap,
                         const uint8_t *src, size_t n) {
    if (*len + n > *cap) {
        *cap = (*len + n) * 2 + 1024;
        *buf = realloc(*buf, *cap);
    }
    memcpy(*buf + *len, src, n);
    *len += n;
}

static const uint8_t kStartCode[4] = {0, 0, 0, 1};

                                                                                    
static void output_callback(void *refcon, void *frame_refcon, OSStatus status,
                            VTEncodeInfoFlags flags, CMSampleBufferRef sample) {
    PwVtEncoder *enc = (PwVtEncoder *)refcon;
    PwOutput *out = &enc->pending;
    out->status = status;
    out->data = NULL;
    out->len = 0;
    out->is_keyframe = 0;

    if (status == noErr && sample != NULL) {
        CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sample, false);
        int32_t keyframe = 1;
        if (attachments && CFArrayGetCount(attachments) > 0) {
            CFDictionaryRef att = CFArrayGetValueAtIndex(attachments, 0);
            keyframe = !CFDictionaryContainsKey(att, kCMSampleAttachmentKey_NotSync);
        }
        out->is_keyframe = keyframe;

        uint8_t *buf = NULL;
        size_t len = 0, cap = 0;

        if (keyframe) {
            CMFormatDescriptionRef fmt = CMSampleBufferGetFormatDescription(sample);
            size_t ps_count = 0;
            CMVideoFormatDescriptionGetHEVCParameterSetAtIndex(fmt, 0, NULL, NULL,
                                                               &ps_count, NULL);
            for (size_t i = 0; i < ps_count; i++) {
                const uint8_t *ps = NULL;
                size_t ps_len = 0;
                if (CMVideoFormatDescriptionGetHEVCParameterSetAtIndex(
                        fmt, i, &ps, &ps_len, NULL, NULL) == noErr) {
                    append_bytes(&buf, &len, &cap, kStartCode, 4);
                    append_bytes(&buf, &len, &cap, ps, ps_len);
                }
            }
        }

        CMBlockBufferRef block = CMSampleBufferGetDataBuffer(sample);
        size_t total = CMBlockBufferGetDataLength(block);
        uint8_t *raw = malloc(total);
        CMBlockBufferCopyDataBytes(block, 0, total, raw);
                                                       
        size_t pos = 0;
        while (pos + 4 <= total) {
            uint32_t nal_len = ((uint32_t)raw[pos] << 24) | ((uint32_t)raw[pos + 1] << 16) |
                               ((uint32_t)raw[pos + 2] << 8) | raw[pos + 3];
            if (pos + 4 + nal_len > total) break;
            append_bytes(&buf, &len, &cap, kStartCode, 4);
            append_bytes(&buf, &len, &cap, raw + pos + 4, nal_len);
            pos += 4 + nal_len;
        }
        free(raw);
        out->data = buf;
        out->len = len;
    }
    dispatch_semaphore_signal(enc->done);
}

                                                                                          
                                                            
PwVtEncoder *pw_vt_create_ex(int32_t width, int32_t height, int32_t gop,
                             double quality, int64_t avg_bitrate,
                             int32_t power_efficient);

PwVtEncoder *pw_vt_create(int32_t width, int32_t height, int32_t gop,
                          double quality, int64_t avg_bitrate) {
    return pw_vt_create_ex(width, height, gop, quality, avg_bitrate, 1);
}

PwVtEncoder *pw_vt_create_ex(int32_t width, int32_t height, int32_t gop,
                             double quality, int64_t avg_bitrate,
                             int32_t power_efficient) {
    PwVtEncoder *enc = calloc(1, sizeof(PwVtEncoder));
    enc->done = dispatch_semaphore_create(0);
    enc->width = width;
    enc->height = height;
    OSStatus st = VTCompressionSessionCreate(
        NULL, width, height, kCMVideoCodecType_HEVC, NULL, NULL, NULL,
        output_callback, enc, &enc->session);
    if (st != noErr) { free(enc); return NULL; }

    VTSessionSetProperty(enc->session, kVTCompressionPropertyKey_ProfileLevel,
                         kVTProfileLevel_HEVC_Main_AutoLevel);
    VTSessionSetProperty(enc->session, kVTCompressionPropertyKey_RealTime,
                         kCFBooleanFalse);
                                                                                          
                                                                                      
                                                                                                   
    VTSessionSetProperty(enc->session,
                         kVTCompressionPropertyKey_MaximizePowerEfficiency,
                         power_efficient ? kCFBooleanTrue : kCFBooleanFalse);
    VTSessionSetProperty(enc->session, kVTCompressionPropertyKey_AllowFrameReordering,
                         kCFBooleanFalse);
    CFNumberRef g = CFNumberCreate(NULL, kCFNumberSInt32Type, &gop);
    VTSessionSetProperty(enc->session, kVTCompressionPropertyKey_MaxKeyFrameInterval, g);
    CFRelease(g);
    if (avg_bitrate > 0) {
        int32_t br = (int32_t)avg_bitrate;
        CFNumberRef b = CFNumberCreate(NULL, kCFNumberSInt32Type, &br);
        VTSessionSetProperty(enc->session, kVTCompressionPropertyKey_AverageBitRate, b);
        CFRelease(b);
    } else {
        CFNumberRef q = CFNumberCreate(NULL, kCFNumberDoubleType, &quality);
        VTSessionSetProperty(enc->session, kVTCompressionPropertyKey_Quality, q);
        CFRelease(q);
    }
    VTCompressionSessionPrepareToEncodeFrames(enc->session);
    return enc;
}

                                                                                                 
int32_t pw_vt_encode_nv12(PwVtEncoder *enc, const uint8_t *y, const uint8_t *uv,
                          int64_t pts_ms, int64_t duration_ms,
                          uint8_t **out_data, int64_t *out_len,
                          int32_t *out_keyframe) {
    CVPixelBufferRef pb = NULL;
    CVReturn cv = CVPixelBufferCreate(NULL, enc->width, enc->height,
        kCVPixelFormatType_420YpCbCr8BiPlanarFullRange, NULL, &pb);
    if (cv != kCVReturnSuccess) return -100;

    CVPixelBufferLockBaseAddress(pb, 0);
    uint8_t *dst_y = CVPixelBufferGetBaseAddressOfPlane(pb, 0);
    size_t stride_y = CVPixelBufferGetBytesPerRowOfPlane(pb, 0);
    for (int r = 0; r < enc->height; r++)
        memcpy(dst_y + r * stride_y, y + (size_t)r * enc->width, enc->width);
    uint8_t *dst_uv = CVPixelBufferGetBaseAddressOfPlane(pb, 1);
    size_t stride_uv = CVPixelBufferGetBytesPerRowOfPlane(pb, 1);
    for (int r = 0; r < enc->height / 2; r++)
        memcpy(dst_uv + r * stride_uv, uv + (size_t)r * enc->width, enc->width);
    CVPixelBufferUnlockBaseAddress(pb, 0);

    CMTime pts = CMTimeMake(pts_ms, 1000);
    CMTime dur = CMTimeMake(duration_ms, 1000);
    OSStatus st = VTCompressionSessionEncodeFrame(enc->session, pb, pts, dur,
                                                  NULL, NULL, NULL);
    CVPixelBufferRelease(pb);
    if (st != noErr) return st;

                                                                              
    dispatch_semaphore_wait(enc->done, DISPATCH_TIME_FOREVER);
    if (enc->pending.status != noErr) return enc->pending.status;
    *out_data = enc->pending.data;
    *out_len = (int64_t)enc->pending.len;
    *out_keyframe = enc->pending.is_keyframe;
    return 0;
}

int32_t pw_vt_flush(PwVtEncoder *enc) {
    return VTCompressionSessionCompleteFrames(enc->session, kCMTimeInvalid);
}

void pw_vt_free(uint8_t *p) { free(p); }

void pw_vt_destroy(PwVtEncoder *enc) {
    if (!enc) return;
    if (enc->session) {
        VTCompressionSessionInvalidate(enc->session);
        CFRelease(enc->session);
    }
    free(enc);
}

                                                       
                                                                                      
                                                                                            
int32_t pw_vt_encode_cvpb(PwVtEncoder *enc, void *pixel_buffer,
                          int64_t pts_ms, int64_t duration_ms,
                          uint8_t **out_data, int64_t *out_len,
                          int32_t *out_keyframe) {
    CVPixelBufferRef pb = (CVPixelBufferRef)pixel_buffer;
    CMTime pts = CMTimeMake(pts_ms, 1000);
    CMTime dur = CMTimeMake(duration_ms, 1000);
    OSStatus st = VTCompressionSessionEncodeFrame(enc->session, pb, pts, dur,
                                                  NULL, NULL, NULL);
    if (st != noErr) return st;
    dispatch_semaphore_wait(enc->done, DISPATCH_TIME_FOREVER);
    if (enc->pending.status != noErr) return enc->pending.status;
    *out_data = enc->pending.data;
    *out_len = (int64_t)enc->pending.len;
    *out_keyframe = enc->pending.is_keyframe;
    return 0;
}
