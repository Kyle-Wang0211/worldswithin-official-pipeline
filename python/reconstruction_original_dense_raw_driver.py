"""Synchronous raw IO only; exact selected numerical source bytes execute in np/cv2."""
import builtins, hashlib, io, json, os, pathlib, struct, sys, types
_SOURCES = {'filter': {'filename': 'selected_original_filter.py', 'full_sha256': 'b3c063519ff8afd8d4bfc607dc97e9edc177afc7f48893c36a1d74019cc1776c', 'bytes': 17110}, 'fusion': {'filename': 'selected_original_fuse_verbatim_sat_angle.py', 'full_sha256': 'b29a8abfb40f6957dbf119bd567ef701437e8d8208d4cc32d9947a882723ceab', 'bytes': 4215}, 'multires': {'filename': 'selected_original_make_multires.py', 'full_sha256': 'ba2608ea79d4f375a36f2422a3c1511093569d1491d5b71abce6e7e1bc5b2b1d', 'bytes': 1198}}
_SPANS = {'filter': {'source': 'filter', 'lo': 8, 'hi': 105, 'sha256': 'af940aef038bde06ddc7bfb7de2d7ea7ab6b088c965929102f5e42bf421fe922', 'bytes': 3551}, 'merge': {'source': 'multires', 'lo': 12, 'hi': 22, 'sha256': '19f28f0b8b65dfc1337fdd0895d2350d9f83227149057c4cbb306dfb2ca7cb06', 'bytes': 759}, 'fusion_setup': {'source': 'fusion', 'lo': 33, 'hi': 35, 'sha256': '8839022f2f6b7fe3bf8460e61a297ec9a48fd7859a79e758afcf705416883545', 'bytes': 242}, 'fusion_defs': {'source': 'fusion', 'lo': 36, 'hi': 74, 'sha256': 'dc0a17e1d586187ba0cff65edcf7bbb23b021c33d0f8ebf2c20f3b88e4dee21b', 'bytes': 2511}, 'fusion_main': {'source': 'fusion', 'lo': 75, 'hi': 85, 'sha256': '5ef75680829c4270d577a9bcf782025c12deea4c482daf8b4b6b6c6dc4915077', 'bytes': 719}}
_FX = "/reconstruction-original-raw/fixture"
_B = "/reconstruction-original-raw/pred"
_LO = "/reconstruction-original-raw/low"
_HI = "/reconstruction-original-raw/high"
_MERGE_OUT = "/reconstruction-original-raw/merged"

def source_codes():
    sources = {}
    for key, row in _SOURCES.items():
        raw = pathlib.Path(__file__).with_name(row["filename"]).read_bytes()
        if len(raw) != row["bytes"] or hashlib.sha256(raw).hexdigest() != row["full_sha256"]:
            raise RuntimeError("original full-source SHA mismatch: " + key)
        sources[key] = raw
    result = {}
    for key, row in _SPANS.items():
        raw = b"".join(sources[row["source"]].splitlines(keepends=True)[row["lo"]-1:row["hi"]])
        if len(raw) != row["bytes"] or hashlib.sha256(raw).hexdigest() != row["sha256"]:
            raise RuntimeError("original numerical-span SHA mismatch: " + key)
        result[key] = compile(raw, _SOURCES[row["source"]]["filename"]+":"+str(row["lo"]), "exec")
    return result

def dependencies(fusion=False):
    if sys.version_info[:3] != (3, 10, 21) or sys.byteorder != "little":
        raise RuntimeError("pinned CPython3.10.21/little-endian required")
    import numpy as np
    import cv2
    if np.__version__ != "1.26.4" or cv2.__version__ != "4.11.0":
        raise RuntimeError("pinned official NumPy1.26.4/OpenCV4.11.0 required")
    Image = None
    if fusion:
        import PIL
        from PIL import Image
        if PIL.__version__ != "12.3.0":
            raise RuntimeError("original Pillow12.3.0 required")
    return np, cv2, Image

def validate_dependencies():
    """Pre-model readiness only: exact source pins and real pinned imports."""
    source_codes()
    helper = pathlib.Path(__file__).with_name("reconstruction_readonly_fusion_io.py")
    if hashlib.sha256(helper.read_bytes()).hexdigest() != "00221bf0530c203a7465badec8729d75e473172ed42fb69d810c64cfb3560393":
        raise RuntimeError("readonly fusion IO adapter SHA mismatch")
    import reconstruction_readonly_fusion_io
    dependencies(fusion=True)
    return None

def checked_view(view, size, writable=False):
    if not isinstance(view, memoryview) or not view.c_contiguous or view.nbytes != size or (writable and view.readonly) or (not writable and not view.readonly):
        raise ValueError("synchronous raw memoryview ownership/length mismatch")
    return view

def transport_frame_count(cameras):
    if not isinstance(cameras, memoryview) or cameras.nbytes % (36 * 4):
        raise ValueError("exact camera row byte length required")
    count = cameras.nbytes // (36 * 4)
    if count <= 0:
        raise ValueError("actual positive frame count required")
    checked_view(cameras, count * 36 * 4)
    return count

def plane(np, value):
    if not isinstance(value, tuple) or len(value) != 3:
        raise ValueError("raw plane tuple required")
    view, width, height = value
    if type(width) is not int or type(height) is not int or width <= 0 or height <= 0:
        raise ValueError("positive plane dimensions required")
    checked_view(view, width*height*4)
    return np.frombuffer(view, dtype="<f4").reshape(height, width)

def copy_array(np, array, target, dtype, shape):
    if array.dtype != np.dtype(dtype) or array.shape != shape or not array.flags.c_contiguous:
        raise ValueError("original numerical output ABI/shape mismatch")
                                                            
    checked_view(target, array.nbytes, writable=True).cast("B")[:] = memoryview(array).cast("B")

class ArrayIO:
    def __init__(self, np, loads, saves):
        self.np, self.loads, self.saves = np, loads, saves
    def __getattr__(self, name):
        return getattr(self.np, name)
    def load(self, path):
        if path not in self.loads:
            raise ValueError("unexpected original np.load path")
        return self.loads[path]
    def save(self, path, array):
        if path not in self.saves:
            raise ValueError("unexpected original np.save path")
        target, shape = self.saves[path]
        copy_array(self.np, array, target, "<f4", shape)

                                                                              
                                                                             
def merge_frame(low, high, outputs, frame_index):
    code = source_codes(); np, cv2, _ = dependencies()
    if type(frame_index) is not int or frame_index < 0 or any(not isinstance(x, tuple) or len(x) != 4 for x in (low, high, outputs)):
        raise ValueError("nonnegative production frame index and four planes required")
    loads, saves = {}, {}
    target_shape = (high[0][2], high[0][1])
    for k, sub in enumerate(("depth", "conf0", "conf1", "conf2")):
        loads[f"{_LO}/{sub}/{frame_index:04d}.npy"] = plane(np, low[k])
        loads[f"{_HI}/{sub}/{frame_index:04d}.npy"] = plane(np, high[k])
        saves[f"{_MERGE_OUT}/{sub}/{frame_index:04d}.npy"] = (outputs[k], target_shape)
    adapter = ArrayIO(np, loads, saves)
    def selected_range(n):
        return (frame_index,) if n == 97 else builtins.range(n)
    scope = {"np": adapter, "cv2": cv2, "LO": _LO, "HI": _HI, "OUT": _MERGE_OUT,
             "TH": 0.02, "share": [], "range": selected_range}
    try:
        exec(code["merge"], scope)
                                                                                   
        return int(np.count_nonzero(scope["mask"]))
    finally:
        scope.clear(); loads.clear(); saves.clear()

def filter_pair(ref, src, kr, er, ks, es, depth_max, depth_min, outputs):
    code = source_codes(); np, cv2, _ = dependencies()
    ref_array, src_array = plane(np, ref), plane(np, src)
    cameras = []
    for view, count, shape in ((kr,9,(3,3)),(er,16,(4,4)),(ks,9,(3,3)),(es,16,(4,4))):
        cameras.append(np.frombuffer(checked_view(view,count*8),dtype="<f8").reshape(shape))
    if not isinstance(outputs, tuple) or len(outputs) != 4:
        raise ValueError("mask/depth/xsrc/ysrc output tuple required")
    scope = {"np": np, "cv2": cv2}
    try:
        exec(code["filter"], scope)
        result = scope["check_geometric_consistency"](ref_array,*cameras[:2],src_array,*cameras[2:],depth_max,depth_min,0.125,0.01)
        for array, target, dtype in zip(result,outputs,("bool","<f4","<f4","<f4")):
            copy_array(np,array,target,dtype,ref_array.shape)
    finally:
        scope.clear()

class SynchronousOrderedExecutor:
    """IO scheduling adapter only; original ex.map consumes original ascending range."""
    def __init__(self, workers):
        if workers != 8:
            raise ValueError("original executor declaration changed")
    def __enter__(self): return self
    def __exit__(self, *_): return False
    def map(self, fn, values): return builtins.map(fn, values)

class FusionArrayIO(ArrayIO):
    def load(self, path):
                                                                                  
                                                                                           
        from reconstruction_readonly_fusion_io import ReadonlyFloat32Load
        return ReadonlyFloat32Load(self.np, super().load(path))
    def __init__(self,np,loads,cameras,retained,frame_ids):
        super().__init__(np,loads,{})
        self.cameras,self.retained,self.frame_ids = cameras,pathlib.Path(retained),frame_ids
        self.frame_count = transport_frame_count(cameras)
        self.written = set()
    def fromfile(self,path,dtype):
        if path != _FX+"/cams.f32" or dtype != self.np.float32:
            raise ValueError("unexpected original np.fromfile IO")
        return self.np.frombuffer(self.cameras,dtype="<f4")
    def savez(self,path,**arrays):
        p=pathlib.PurePosixPath(path)
        if str(p.parent) != "/reconstruction-original-raw/output_m2" or len(p.stem)!=8 or not p.stem.isdigit():
            raise ValueError("unexpected original np.savez IO")
        index=int(p.stem)
        if not 0<=index<self.frame_count or index in self.written or set(arrays)!={"d16","col","intr","extr"}:
            raise ValueError("original retention field/order mismatch")
        for name,dtype,shape in (("d16","<u2",(1152,1536)),("col","u1",(1152,1536,3)),("intr","<f8",(3,3)),("extr","<f8",(4,4))):
            array=arrays[name]
            if array.dtype!=self.np.dtype(dtype) or array.shape!=shape or not array.flags.c_contiguous:
                raise ValueError("original retained array type/layout mismatch: "+name)
        stem=f"{index:08d}"
        for name,suffix in (("d16",".d16"),("col",".col.u8")):
            with (self.retained/(stem+suffix)).open("xb") as stream: arrays[name].tofile(stream)
        with (self.retained/(stem+".camera.f64le")).open("xb") as stream:
            arrays["intr"].tofile(stream);arrays["extr"].tofile(stream)
        metadata={"schema":1,"pack_index":index,"original_index":index,"frame_id_f64_bits":self.frame_ids[index*8:(index+1)*8].tobytes()[::-1].hex(),
                  "width":1536,"height":1152,"depth_scale":5000,"d16":stem+".d16","d16_bytes":3538944,
                  "camera":stem+".camera.f64le","camera_bytes":200,"camera_layout":"K9_then_E16_row_major_f64le"}
        with (self.retained/(stem+".json")).open("x") as stream:json.dump(metadata,stream,separators=(",",":"));stream.write("\n")
        self.written.add(index)

class RawImageIO:
    def __init__(self,Image,rgb,width,height):
        self.Image,self.rgb,self.width,self.height=Image,rgb,width,height
        self.frame_count = rgb.nbytes // (width*height*3)
    def open(self,path):
        stem=pathlib.PurePosixPath(path).stem
        if str(pathlib.PurePosixPath(path).parent)!=_FX+"/rgb" or len(stem)!=8 or not stem.isdigit() or not 0<=int(stem)<self.frame_count:
            raise ValueError("unexpected original Image.open IO")
        n=self.width*self.height*3;i=int(stem)
                                                                          
        return self.Image.frombytes("RGB",(self.width,self.height),self.rgb[i*n:(i+1)*n].tobytes())
    def __getattr__(self,name): return getattr(self.Image,name)

class OriginalJPEGImageIO:
    """IO path mapping only; original Pillow open/convert/resize stays unchanged."""
    def __init__(self, Image, paths):
        if not isinstance(paths, tuple) or not paths:
            raise ValueError("positive actual-N ordered original JPEG paths required")
        for path in paths:
            if not isinstance(path, str) or not pathlib.Path(path).is_absolute() or not pathlib.Path(path).is_file():
                raise ValueError("existing absolute original JPEG path required")
        self.Image, self.paths = Image, paths
    def open(self, path):
        p = pathlib.PurePosixPath(path)
        if str(p.parent) != _FX+"/rgb" or len(p.stem) != 8 or not p.stem.isdigit() or not 0 <= int(p.stem) < len(self.paths):
            raise ValueError("unexpected original Image.open IO")
        return self.Image.open(self.paths[int(p.stem)])
    def __getattr__(self, name): return getattr(self.Image, name)

class OutputDirectoryIO:
    def __init__(self,retained):self.retained=retained
    def makedirs(self,path,exist_ok=False):
        if path!="/reconstruction-original-raw/output_m2" or not exist_ok:
            raise ValueError("unexpected original output directory IO")
        pathlib.Path(self.retained).mkdir(mode=0o700)

def fuse_pack(banks,cameras,rgb,rgb_width,rgb_height,frame_ids,retained,fused_ply,original_jpeg_paths=None):
    code=source_codes()
                                                                          
                                                                   
    if fused_ply is not None:
        raise RuntimeError("original selected fusedPLY producer identity unavailable; retained-only bridge supported")
    np,cv2,Image=dependencies(fusion=True)
    if not isinstance(banks,tuple) or len(banks)!=4 or type(rgb_width)is not int or type(rgb_height)is not int :
        raise ValueError("four actual-N frame banks and source RGB grid required")
    count=transport_frame_count(cameras)
    checked_view(frame_ids,count*8)
    if original_jpeg_paths is not None:
        if len(original_jpeg_paths) != count:
            raise ValueError("original JPEG and camera frame counts differ")
        image_io=OriginalJPEGImageIO(Image,original_jpeg_paths)
    else:
                                                                                 
        if rgb_width<=0 or rgb_height<=0:raise ValueError("diagnostic RGB grid required")
        checked_view(rgb,count*rgb_width*rgb_height*3)
        image_io=RawImageIO(Image,rgb,rgb_width,rgb_height)
    target=pathlib.Path(retained)
    if not isinstance(retained,str) or not target.is_absolute() or target.exists() or target.is_symlink():
        raise ValueError("fresh absolute retained directory required")
    loads={}
    for k,sub in enumerate(("depth","conf0","conf1","conf2")):
        checked_view(banks[k],count*1536*1152*4)
        bank=np.frombuffer(banks[k],dtype="<f4").reshape(count,1152,1536)
        for i in range(count):loads[f"{_B}/{sub}/{i:04d}.npy"]=bank[i]
    adapter=FusionArrayIO(np,loads,cameras,retained,frame_ids)
    def original_open(path,mode="r",*args,**kwargs):
        if path==_FX+"/frames.json" and mode=="r":return io.StringIO(json.dumps({"count":count,"width":1536,"height":1152}))
        if path=="/reconstruction-original-raw/output_m2/order.json" and mode=="w":return (target/"order.json").open("x",*args,**kwargs)
        raise ValueError("unexpected original open IO")
    scope={"np":adapter,"Image":image_io,"json":json,"open":original_open,
           "os":OutputDirectoryIO(retained),"ProcessPoolExecutor":SynchronousOrderedExecutor,
           "FX":_FX,"B":_B,"OUT":"/reconstruction-original-raw/output",
           "a":types.SimpleNamespace(geo_pixel=0.125,mask_thres=[2],sat_tol=1e-5,min_tri_angle=3.0),"__name__":"__main__"}
    filter_scope={"np":np,"cv2":cv2}
    try:
        exec(code["filter"],filter_scope);scope["check_geometric_consistency"]=filter_scope["check_geometric_consistency"]
                                                                                                      
        exec(code["fusion_setup"],scope);exec(code["fusion_defs"],scope);exec(code["fusion_main"],scope)
        if adapter.written!=set(range(count)):raise RuntimeError("incomplete original retained outputs")
        return int(scope["tot"][2])
    finally:
        scope.clear();filter_scope.clear();loads.clear()
