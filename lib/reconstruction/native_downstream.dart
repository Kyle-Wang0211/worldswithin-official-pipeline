import 'dart:convert';
import 'dart:ffi';
import 'dart:io';
import 'request.dart';
import 'original_retention_io.dart';
import 'vendor/dart_pipeline/lib/reconstruction_pipeline.dart';
import 'vendor/dart_pipeline/lib/reconstruction_geometry_ffi.dart';
import 'vendor/dart_texture_integration_v1/lib/reconstruction_texture_ffi.dart';

final class TsdfFrame extends Struct {
  @UintPtr() external int width;
  @UintPtr() external int height;
  @Array(9) external Array<Double> intrinsics;
  @Array(16) external Array<Double> extrinsics;
}
final class NormalFrame extends Struct {
  external Pointer<Char> d16;
  @Uint32() external int width;
  @Uint32() external int height;
  @Array(9) external Array<Double> intrinsics;
  @Array(16) external Array<Double> extrinsics;
  external Pointer<Char> originalCamera;
}
typedef CreateN = Int32 Function(Pointer<Pointer<Void>>);
typedef CreateD = int Function(Pointer<Pointer<Void>>);
typedef DestroyN = Void Function(Pointer<Void>);
typedef DestroyD = void Function(Pointer<Void>);
typedef IntegrateN = Int32 Function(Pointer<Void>,Pointer<Char>,Pointer<TsdfFrame>,Pointer<UintPtr>);
typedef IntegrateD = int Function(Pointer<Void>,Pointer<Char>,Pointer<TsdfFrame>,Pointer<UintPtr>);
typedef OriginalIntegrateXyzN = Int32 Function(Pointer<Void>,Pointer<Double>,UintPtr,Pointer<Double>);
typedef OriginalD16N = Int32 Function(Pointer<Void>,Pointer<NativeFunction<OriginalIntegrateXyzN>>,Pointer<Uint16>,UintPtr,UintPtr,Pointer<Double>,Pointer<Double>,Pointer<UintPtr>);
typedef OriginalD16D = int Function(Pointer<Void>,Pointer<NativeFunction<OriginalIntegrateXyzN>>,Pointer<Uint16>,int,int,Pointer<Double>,Pointer<Double>,Pointer<UintPtr>);
typedef ExtractN = Int32 Function(Pointer<Void>,Pointer<Pointer<Void>>);
typedef ExtractD = int Function(Pointer<Void>,Pointer<Pointer<Void>>);
typedef CountN = UintPtr Function(Pointer<Void>);
typedef CountD = int Function(Pointer<Void>);
typedef VerticesN = Pointer<Double> Function(Pointer<Void>);
typedef VerticesD = Pointer<Double> Function(Pointer<Void>);
typedef FacesN = Pointer<Int32> Function(Pointer<Void>);
typedef FacesD = Pointer<Int32> Function(Pointer<Void>);
typedef ExportN = Int32 Function(Pointer<Double>,UintPtr,Pointer<Int32>,UintPtr,Pointer<Char>,Pointer<UintPtr>);
typedef ExportD = int Function(Pointer<Double>,int,Pointer<Int32>,int,Pointer<Char>,Pointer<UintPtr>);
typedef CleanN = Int32 Function(Pointer<Char>,Pointer<Char>,Pointer<UintPtr>);
typedef CleanD = int Function(Pointer<Char>,Pointer<Char>,Pointer<UintPtr>);
typedef ReadN = Int32 Function(Pointer<Char>,Pointer<Pointer<Void>>);
typedef ReadD = int Function(Pointer<Char>,Pointer<Pointer<Void>>);
typedef HasColorsN = Int32 Function(Pointer<Void>);
typedef HasColorsD = int Function(Pointer<Void>);
typedef OriginalVisibilityN = Int32 Function(Pointer<Double>,Pointer<Double>,Uint64,Int32,Pointer<NormalFrame>,Uint32,Pointer<Char>,Pointer<Char>,Pointer<Char>);
typedef OriginalVisibilityD = int Function(Pointer<Double>,Pointer<Double>,int,int,Pointer<NormalFrame>,int,Pointer<Char>,Pointer<Char>,Pointer<Char>);
typedef PairN = Int32 Function(Pointer<Char>,Pointer<Char>);
typedef PairD = int Function(Pointer<Char>,Pointer<Char>);
typedef NormalN = Int32 Function(Pointer<Char>,Pointer<Char>,Pointer<Char>,Pointer<Char>,Pointer<Char>);
typedef NormalD = int Function(Pointer<Char>,Pointer<Char>,Pointer<Char>,Pointer<Char>,Pointer<Char>);
typedef TripleN = Int32 Function(Pointer<Char>,Pointer<Char>,Pointer<Char>);
typedef TripleD = int Function(Pointer<Char>,Pointer<Char>,Pointer<Char>);
                                                                                
typedef MeshoptPlyN = Int32 Function(Pointer<Char>,Pointer<Char>,Pointer<Char>,Pointer<Char>,
  Pointer<Char>,Pointer<Char>,Pointer<Uint64>,Pointer<Uint64>,Pointer<Uint64>,Pointer<Uint64>);
typedef MeshoptPlyD = int Function(Pointer<Char>,Pointer<Char>,Pointer<Char>,Pointer<Char>,
  Pointer<Char>,Pointer<Char>,Pointer<Uint64>,Pointer<Uint64>,Pointer<Uint64>,Pointer<Uint64>);


void copyF64(Array<Double> array,List<int> bytes) {
  final target=array.elements;
  if(bytes.length!=target.lengthInBytes){throw StateError("Frozen camera array byte length mismatch");}
  target.buffer.asUint8List(target.offsetInBytes,target.lengthInBytes).setAll(0,bytes);
}

final class Arena {
  Arena(this.memory);
  final ReconstructionNativeMemory memory;
  final owned = <Pointer<Uint8>>[];
  Pointer<Uint8> bytes(int count) {
    final p = memory.allocate(count); owned.add(p);
    p.asTypedList(count).fillRange(0,count,0); return p;
  }
  Pointer<Char> path(String path) {
    reconstructionTextureAbsolutePath(path);
    final data=utf8.encode(path);final p=bytes(data.length+1);
    p.asTypedList(data.length).setAll(0,data);return p.cast<Char>();
  }
  void dispose() { for(final p in owned.reversed){memory.release(p);} owned.clear(); }
}

                                                                              
final class NativeSlot {
  NativeSlot(this.library,this.binarySha256,this.symbols,this.sourceEvidence);
  final DynamicLibrary library;
  final String binarySha256;
  final Set<String> symbols;
  final Map<String,dynamic> sourceEvidence;
  static Future<NativeSlot> open(Map<String,dynamic> row, String bundleRoot, Set<String> required) async {
    final relative=checkedRelativePath(string(row,'bundle_relative_path'));
    final file=File('$bundleRoot/$relative');
    final expected=string(row,'binary_sha256');
    if(await hashFile(file)!=expected) {throw StateError('Native library bytes changed: $relative');}
    final receipt=File('$bundleRoot/${checkedRelativePath(string(row,'source_receipt_path'))}');
    if(await hashFile(receipt)!=string(row,'source_receipt_sha256')) {
      throw StateError('Root source-bound native receipt changed: $relative');
    }
    final evidence=object(jsonDecode(await receipt.readAsString()));
    if(evidence['accepted_source_closure']!=true || evidence['binary_sha256']!=expected) {
      throw UnsupportedError('Unaccepted source closure: $relative');
    }
    final defined=(evidence['actual_defined_c_symbols'] as List).cast<String>().toSet();
    if(!defined.containsAll(required)) {throw UnsupportedError('Missing real native functions: ${required.difference(defined)}');}
    final library=DynamicLibrary.open(file.path);
    for(final symbol in required) {library.lookup<NativeFunction<Void Function()>>(symbol);}
    return NativeSlot(library,expected,defined,Map<String,dynamic>.unmodifiable(evidence));
  }
}

final class NativeDriver implements ReconstructionStageDriver {
  NativeDriver(this.stage,this.slot,this.symbols,this.call);
  @override final ReconstructionStage stage;
  final NativeSlot slot;
  final List<String> symbols;
  final Future<int> Function() call;
  @override String get sourceManifestSha256 => reconstructionSourceInputManifestSha256;
  @override String get binarySha256 => slot.binarySha256;
  @override List<String> get verifiedCAbiSymbols => List.unmodifiable(symbols);
  @override Future<int> run(ReconstructionRunContract contract) => call();
}

final class DownstreamNative {
  DownstreamNative(this.request,this.bundleRoot, {
    List<Map<String,Object?>>? calls,
    this.retainedFramesFromInput = false,
    this.allowDiagnosticTexture = false,
  }) : calls=calls ?? [];
                                                                          
                                                                      
  final bool retainedFramesFromInput;
                                                                               
  final bool allowDiagnosticTexture;
  final List<Map<String,Object?>> calls;
  int record(String entry,int rc,[Map<String,Object?>? details]) {
    calls.add({'native_entry':entry,'return_code':rc,...?details});return rc;
  }
  final ReconstructionRequestTransport request;
  final String bundleRoot;
  final memory=ReconstructionSystemMemory(DynamicLibrary.process());
  final drivers=<ReconstructionStage,ReconstructionStageDriver>{};
  final missing=<ReconstructionStage,String>{};
  NativeSlot? _pythonRuntime;
  String input(String relative) => '${request.ioRoots.durableInputRoot}/${checkedRelativePath(relative)}';
  String output(String relative) => '${request.ioRoots.downstreamOutputRoot}/${checkedRelativePath(relative)}';
  String path(ReconstructionStage stage,String name,bool isInput) {
    final p=string(object(request.paths[stage.name]),name);
    return isInput?input(p):output(p);
  }
  Future<NativeSlot> slot(String name,Set<String> symbols) async {
    final row=request.native[name];
    if(row==null) {throw UnsupportedError('$name: no accepted iOS native library in package');}
    return NativeSlot.open(object(row),bundleRoot,symbols);
  }
  String retainedOutput(int index,String key) {
    final relative = request.retainedOutputPath(index,key);
    return retainedFramesFromInput ? input(relative) : output(relative);
  }
  Future<void> bind({Iterable<ReconstructionStage>? stages}) async {
    final selected = stages?.toSet();
    if (selected != null && (selected.isEmpty ||
        selected.any((s) => !reconstructionStageOrder.skip(4).contains(s)))) {
      throw StateError('A nonempty downstream process segment is required');
    }
    bool needs(ReconstructionStage stage) => selected == null || selected.contains(stage);
    if (needs(ReconstructionStage.tsdf)) await bindTsdf();
    if (needs(ReconstructionStage.trim)) await bindMeshio();
    if (needs(ReconstructionStage.lagrange)) await bindGeometry();
    if (needs(ReconstructionStage.meshopt)) await bindMeshoptPly();
    if (needs(ReconstructionStage.texture)) await bindTexture();
    if ([ReconstructionStage.avVisibility, ReconstructionStage.avSfmToAbc, ReconstructionStage.avNormal].any(needs)) {
      await bindNormal(includeVisibility: needs(ReconstructionStage.avVisibility));
    }
                                                                               
    if (needs(ReconstructionStage.glb) || needs(ReconstructionStage.tsdf) || needs(ReconstructionStage.avVisibility)) await bindGlb();
    if (needs(ReconstructionStage.avVisibility) && _pythonRuntime == null) {
      missing[ReconstructionStage.avVisibility]=missing[ReconstructionStage.glb] ?? 'Pinned CPython runtime is unavailable';
      drivers.remove(ReconstructionStage.avVisibility);
    }
  }
  Future<void> bindTsdf() async {
    const symbols={'reconstruction_vdb_create','reconstruction_vdb_destroy','reconstruction_vdb_integrate_xyz',
      'reconstruction_vdb_extract_raw','reconstruction_vdb_mesh_destroy','reconstruction_vdb_mesh_vertex_count',
      'reconstruction_vdb_mesh_face_count','reconstruction_vdb_mesh_vertices','reconstruction_vdb_mesh_faces'};
    try {
      final s=await slot('tsdf',symbols);final l=s.library;
      drivers[ReconstructionStage.tsdf]=NativeDriver(ReconstructionStage.tsdf,s,symbols.toList(),() async {
        final a=Arena(memory);Pointer<Void> volume=nullptr,mesh=nullptr;
        final destroy=l.lookupFunction<DestroyN,DestroyD>('reconstruction_vdb_destroy');
        final meshDestroy=l.lookupFunction<DestroyN,DestroyD>('reconstruction_vdb_mesh_destroy');
        try {
          final holder=a.bytes(sizeOf<Pointer<Void>>()).cast<Pointer<Void>>();
          var rc=l.lookupFunction<CreateN,CreateD>('reconstruction_vdb_create')(holder);
          record('reconstruction_vdb_create',rc);
          volume=holder.value;if(rc!=0){return rc;}
          final points=a.bytes(sizeOf<UintPtr>()).cast<UintPtr>();
          final pythonRuntime=_pythonRuntime;
          if(pythonRuntime==null){throw StateError('Pinned original TSDF NumPy runtime initialization missing');}
          final integrateXyz=l.lookup<NativeFunction<OriginalIntegrateXyzN>>('reconstruction_vdb_integrate_xyz');
          final integrate=pythonRuntime.library.lookupFunction<OriginalD16N,OriginalD16D>('reconstruction_vdb_integrate_d16_original_numpy');
          for(var index=0;index<request.frames.length;index++) {
            final retained=request.frames[index];
            final camera=await File(retainedOutput(index,'camera_f64')).readAsBytes();
            if(camera.length!=200){throw StateError('Camera f64 identity must contain25 values');}
            final perFrame=Arena(memory);
            try {
                                                                                       
              final cameraMemory=perFrame.bytes(200);
              cameraMemory.asTypedList(200).setAll(0,camera);
              final depthBytes=retained.width*retained.height*2;
              final depthMemory=perFrame.bytes(depthBytes);
              final depthFile=await File(retainedOutput(index,'d16')).open();
              try {
                if(await depthFile.length()!=depthBytes){throw StateError('Original d16 shape/length mismatch');}
                var done=0;
                final view=depthMemory.asTypedList(depthBytes);
                while(done<depthBytes) {
                  final got=await depthFile.readInto(view,done);
                  if(got<=0){throw StateError('Incomplete original d16 input');}
                  done+=got;
                }
              } finally {await depthFile.close();}
              rc=integrate(volume,integrateXyz,depthMemory.cast<Uint16>(),retained.width,retained.height,
                cameraMemory.cast<Double>(),(cameraMemory+72).cast<Double>(),points);
              record('reconstruction_vdb_integrate_d16_original_numpy',rc,{'frame_index':index});
              if(rc!=0){return rc;}
            } finally {perFrame.dispose();}
          }
          rc=l.lookupFunction<ExtractN,ExtractD>('reconstruction_vdb_extract_raw')(volume,holder);
          record('reconstruction_vdb_extract_raw',rc);
          mesh=holder.value;if(rc!=0){return rc;}
          final nv=l.lookupFunction<CountN,CountD>('reconstruction_vdb_mesh_vertex_count')(mesh);
          final nf=l.lookupFunction<CountN,CountD>('reconstruction_vdb_mesh_face_count')(mesh);
          return record('reconstruction_trimesh_process_export_raw',pythonRuntime.library.lookupFunction<ExportN,ExportD>('reconstruction_trimesh_process_export_raw')(
            l.lookupFunction<VerticesN,VerticesD>('reconstruction_vdb_mesh_vertices')(mesh),nv,
            l.lookupFunction<FacesN,FacesD>('reconstruction_vdb_mesh_faces')(mesh),nf,
            a.path(path(ReconstructionStage.tsdf,'output_ply',false)),a.bytes(6*sizeOf<UintPtr>()).cast<UintPtr>()));
        } finally {if(mesh!=nullptr){meshDestroy(mesh);}if(volume!=nullptr){destroy(volume);}a.dispose();}
      });
    } catch(e) {missing[ReconstructionStage.tsdf]='$e';}
  }
  Future<void> bindMeshio() async {
    try {
      final s=await slot('meshio',{'reconstruction_open3d_clean_ply'});
      final l=s.library;
      drivers[ReconstructionStage.trim]=NativeDriver(ReconstructionStage.trim,s,['reconstruction_open3d_clean_ply'],() async {
        final a=Arena(memory);try{return record('reconstruction_open3d_clean_ply',l.lookupFunction<CleanN,CleanD>('reconstruction_open3d_clean_ply')(
          a.path(path(ReconstructionStage.trim,'input_ply',false)),a.path(path(ReconstructionStage.trim,'output_ply',false)),
          a.bytes(6*sizeOf<UintPtr>()).cast<UintPtr>()));}finally{a.dispose();}
      });
    } catch(e) {missing[ReconstructionStage.trim]='$e';}
  }
  Future<void> bindGeometry() async {
    try {
      final s=await slot('geometry',reconstructionGeometrySymbols);
      final ffi=ReconstructionGeometryFfi(ReconstructionGeometryBindings.bind(s.library,ReconstructionGeometryEvidence(
                                                                               
                                                                            
                                                                  
        headerSha256:reconstructionGeometryHeaderSha256,
        binarySha256:string(s.sourceEvidence,'unsigned_provider_sha256'),
        sourceInputManifestSha256:reconstructionSourceInputManifestSha256,actualDefinedSymbols:s.symbols)),memory);
      drivers[ReconstructionStage.lagrange]=NativeDriver(ReconstructionStage.lagrange,s,['reconstruction_lagrange_fill_ply'],() async =>
        record('reconstruction_lagrange_fill_ply',ffi.fillPly(inputPly:path(ReconstructionStage.lagrange,'input_ply',false),
          outputPly:path(ReconstructionStage.lagrange,'output_ply',false)).entryReturnCode));
    } catch(e) {missing[ReconstructionStage.lagrange]='$e';}
  }
  Future<void> bindMeshoptPly() async {
    const symbol='reconstruction_meshopt_simplify_ply';
    try {
      final s=await slot('meshopt_ply',{symbol,'reconstruction_mo_ply_last_error'});
      final config=object(request.paths[ReconstructionStage.meshopt.name]);
      const names=['input_ply','output_ply','positions_f32','indices_u32',
        'output_positions_f32','output_indices_u32'];
      final relative={for(final name in names)name:checkedRelativePath(string(config,name))};
      final trim=request.outputPaths(ReconstructionStage.trim,'mesh_ply');
      if(trim.length!=1 || trim.single!=relative['input_ply']) {
        throw StateError('meshopt input must be this run registered trim PLY output');
      }
      for(final entry in {'mesh_ply':'output_ply','positions_f32':'output_positions_f32',
          'indices_u32':'output_indices_u32'}.entries) {
        final rows=request.outputPaths(ReconstructionStage.meshopt,entry.key);
        if(rows.length!=1 || rows.single!=relative[entry.value]) {
          throw StateError('meshopt ${entry.key} output does not bind ${entry.value}');
        }
      }
      final paths={for(final entry in relative.entries)entry.key:output(entry.value)};
      if(paths.values.toSet().length!=names.length){throw StateError('meshopt six IO paths must be distinct');}
      drivers[ReconstructionStage.meshopt]=NativeDriver(ReconstructionStage.meshopt,s,[symbol],() async {
                                                                                        
        for(final name in names.skip(1)) {
          if(await FileSystemEntity.type(paths[name]!,followLinks:false)!=FileSystemEntityType.notFound) {
            throw StateError('Refuse meshopt raw/PLY overwrite: ${paths[name]}');
          }
        }
        final arena=Arena(memory);
        try {
          final counts=arena.bytes(4*sizeOf<Uint64>()).cast<Uint64>();
          return record(symbol,s.library.lookupFunction<MeshoptPlyN,MeshoptPlyD>(symbol)(
            arena.path(paths['input_ply']!),arena.path(paths['output_ply']!),
            arena.path(paths['positions_f32']!),arena.path(paths['indices_u32']!),
            arena.path(paths['output_positions_f32']!),arena.path(paths['output_indices_u32']!),
            counts,counts+1,counts+2,counts+3));
        }finally{arena.dispose();}
      });
    }catch(e){missing[ReconstructionStage.meshopt]='$e';}
  }
  Future<void> bindTexture() async {
    try {
      final s=await slot('texture',{reconstructionTextureSymbol});
                                                                                
                                                                                  
      final textureRow=object(request.native['texture']);
      final evidence=await ReconstructionTextureEvidence.fromVerifiedSourceReceipt(
          binary:File('$bundleRoot/${string(textureRow,'bundle_relative_path')}'),
          receipt:s.sourceEvidence,
          receiptSha256:string(textureRow,'source_receipt_sha256'),
          signedBinarySha256:s.binarySha256,
          sourceInputManifestSha256:reconstructionSourceInputManifestSha256,
          actualDefinedSymbols:s.symbols);
      evidence.requireExecutionScope(allowDiagnostic:allowDiagnosticTexture);
      final ffi=ReconstructionTextureFfi(ReconstructionTextureBindings.bind(s.library,evidence),memory);
      final p=object(request.paths['texture']);
      if (request is! MainlineRequest || retainedFramesFromInput) {
        drivers[ReconstructionStage.texture]=NativeDriver(ReconstructionStage.texture,s,[reconstructionTextureSymbol],() async {
          final result=ffi.run(sceneDirectory:request.ioRoots.textureSceneRoot,
              meshPly:path(ReconstructionStage.texture,'input_ply',false),
              outputPrefix:path(ReconstructionStage.texture,'output_prefix',false));
          record(reconstructionTextureSymbol,result.entryReturnCode,{'native_error':result.error,'diagnostic_issue':result.diagnosticIssue});
          if(result.entryReturnCode==0 && result.diagnosticIssue!=null){throw StateError(result.diagnosticIssue!);}
          return result.entryReturnCode;
        });
        return;
      }
      drivers[ReconstructionStage.texture]=ReconstructionTextureStageDriver(ffi:ffi,io:ReconstructionTextureIo(
        runId:request.runId,inputRoot:Directory(request.ioRoots.durableInputRoot),
        runRoot:Directory(request.ioRoots.downstreamOutputRoot),
        sceneRelativeDirectory:request.ioRoots.textureSceneRoot.substring(request.ioRoots.durableInputRoot.length+1),
        meshRelativePath:string(p,'input_ply'),outputRelativePrefix:string(p,'output_prefix')));
    } catch(e) {missing[ReconstructionStage.texture]='$e';}
  }
  Future<void> bindNormal({bool includeVisibility = true}) async {
    const symbols={'reconstruction_av_convert_sfm_to_abc','reconstruction_av_bake_normal'};
    try {
      final s=await slot('normal',symbols);
      if (includeVisibility) {
      final visibilitySymbols={'reconstruction_av_prepare_sfm_original_numpy',
        if(usesOriginalRetained(request.json)) 'reconstruction_av_original_numpy_last_error'};
      final visibility=await slot('glb',visibilitySymbols);
      final io=await slot('meshio',{'reconstruction_open3d_read_ply','reconstruction_open3d_mesh_destroy',
        'reconstruction_open3d_vertex_count','reconstruction_open3d_vertices','reconstruction_open3d_has_vertex_colors','reconstruction_open3d_vertex_colors'});
      drivers[ReconstructionStage.avVisibility]=NativeDriver(ReconstructionStage.avVisibility,visibility,visibilitySymbols.toList(),() async {
        final runtime=_pythonRuntime;
        if (runtime==null || runtime.binarySha256!=visibility.binarySha256) {
          throw StateError('Original visibility requires its verified initialized CPython provider');
        }
        final a=Arena(memory);Pointer<Void> mesh=nullptr;
        try {
          final holder=a.bytes(sizeOf<Pointer<Void>>()).cast<Pointer<Void>>();
          var rc=io.library.lookupFunction<ReadN,ReadD>('reconstruction_open3d_read_ply')(
            a.path(path(ReconstructionStage.avVisibility,'high_ply',false)),holder);
          record('reconstruction_open3d_read_ply',rc);
          mesh=holder.value;if(rc!=0){return rc;}
          final count=io.library.lookupFunction<CountN,CountD>('reconstruction_open3d_vertex_count')(mesh);
          final hasColor=io.library.lookupFunction<HasColorsN,HasColorsD>('reconstruction_open3d_has_vertex_colors')(mesh);
          final colors=io.library.lookupFunction<VerticesN,VerticesD>('reconstruction_open3d_vertex_colors')(mesh);
          final frames=a.bytes(request.frames.length*sizeOf<NormalFrame>()).cast<NormalFrame>();
          for(var i=0;i<request.frames.length;i++) {
            final f=request.frames[i];final native=(frames+i).ref;
            native.d16=a.path(retainedOutput(i,'d16'));native.width=f.width;native.height=f.height;
            native.originalCamera=a.path(input(f.originalCameraPath));
            final camera=await File(retainedOutput(i,'camera_f64')).readAsBytes();
            copyF64(native.intrinsics,camera.sublist(0,72));
            copyF64(native.extrinsics,camera.sublist(72));
          }
          rc=visibility.library.lookupFunction<OriginalVisibilityN,OriginalVisibilityD>('reconstruction_av_prepare_sfm_original_numpy')(
            io.library.lookupFunction<VerticesN,VerticesD>('reconstruction_open3d_vertices')(mesh),colors,count,hasColor,frames,request.frames.length,
            a.path(path(ReconstructionStage.avVisibility,'original_images',true)),
            a.path(path(ReconstructionStage.avVisibility,'sfm_json',false)),
            a.path(path(ReconstructionStage.avVisibility,'gate_directory',false)));
                                                                             
                                                                                 
          String? nativeError;
          if (rc != 0 && visibility.symbols.contains('reconstruction_av_original_numpy_last_error')) {
            final error=visibility.library.lookupFunction<ReconstructionGeometryErrorNative,ReconstructionGeometryErrorDart>('reconstruction_av_original_numpy_last_error')();
            if (error == nullptr) {
              nativeError='Original visibility error getter returned nullptr';
            } else {
              final bytes=error.cast<Uint8>().asTypedList(2048);
              final end=bytes.indexOf(0);
              nativeError=utf8.decode(bytes.sublist(0,end < 0 ? bytes.length : end),allowMalformed:true);
            }
          }
          return record('reconstruction_av_prepare_sfm_original_numpy',rc,{'native_error':nativeError});
        }finally{if(mesh!=nullptr){io.library.lookupFunction<DestroyN,DestroyD>('reconstruction_open3d_mesh_destroy')(mesh);}a.dispose();}
      });
      }
      drivers[ReconstructionStage.avSfmToAbc]=NativeDriver(ReconstructionStage.avSfmToAbc,s,['reconstruction_av_convert_sfm_to_abc'],() async {
        final a=Arena(memory);try{return record('reconstruction_av_convert_sfm_to_abc',s.library.lookupFunction<PairN,PairD>('reconstruction_av_convert_sfm_to_abc')(
          a.path(path(ReconstructionStage.avSfmToAbc,'sfm_json',false)),a.path(path(ReconstructionStage.avSfmToAbc,'sfm_abc',false))));
        }finally{a.dispose();}
      });
      drivers[ReconstructionStage.avNormal]=NativeDriver(ReconstructionStage.avNormal,s,['reconstruction_av_bake_normal'],() async {
        final a=Arena(memory);try{return record('reconstruction_av_bake_normal',s.library.lookupFunction<NormalN,NormalD>('reconstruction_av_bake_normal')(
          a.path(path(ReconstructionStage.avNormal,'sfm_abc',false)),a.path(path(ReconstructionStage.avNormal,'low_obj',false)),
          a.path(path(ReconstructionStage.avNormal,'high_ply',false)),a.path(path(ReconstructionStage.avNormal,'original_images',true)),
          a.path(path(ReconstructionStage.avNormal,'normal_directory',false))));}finally{a.dispose();}
      });
    }catch(e){for(final stage in [ReconstructionStage.avVisibility,ReconstructionStage.avSfmToAbc,ReconstructionStage.avNormal]){missing[stage]='$e';}}
  }
                                                                                   
  Future<void> initializePythonRuntime() async {
    await bindGlb();
    if (_pythonRuntime == null) {
      throw StateError(missing[ReconstructionStage.glb] ?? 'Pinned original CPython runtime unavailable');
    }
  }
  Future<void> bindGlb() async {
    if (_pythonRuntime != null) return;
    try {
      const symbols={'reconstruction_glb_runtime_initialize','reconstruction_glb_export_obj','reconstruction_trimesh_process_export_raw','reconstruction_vdb_integrate_d16_original_numpy','reconstruction_av_prepare_sfm_original_numpy'};
      final s=await slot('glb',symbols);
      final row=object(request.native['glb']);
      final home='$bundleRoot/${checkedRelativePath(string(row,'runtime_home_bundle_relative_path'))}';
      final modules='$bundleRoot/${checkedRelativePath(string(row,'native_modules_bundle_relative_path'))}';
      final initArena=Arena(memory);
      try {
        final rc=s.library.lookupFunction<PairN,PairD>('reconstruction_glb_runtime_initialize')(
          initArena.path(home),initArena.path(modules));
        record('reconstruction_glb_runtime_initialize',rc);
        if(rc!=0){throw StateError('Pinned CPython initialization returned $rc');}
      } finally {initArena.dispose();}
      _pythonRuntime=s;
      drivers[ReconstructionStage.glb]=NativeDriver(ReconstructionStage.glb,s,symbols.toList(),() async {
        final a=Arena(memory);try{return record('reconstruction_glb_export_obj',s.library.lookupFunction<TripleN,TripleD>('reconstruction_glb_export_obj')(
          a.path(path(ReconstructionStage.glb,'obj',false)),a.path(path(ReconstructionStage.glb,'normal_directory',false)),
          a.path(path(ReconstructionStage.glb,'output_glb',false))));}finally{a.dispose();}
      });
    }catch(e){missing[ReconstructionStage.glb]='$e';}
  }
}
