import { sourceWorldBounds, createSelectionPreview } from './publication_selection.mjs';
import { createSurfaceComments } from './surface_comments.mjs';
import { useOfficialTextureLoader } from './image_loader_lifecycle.mjs';
import { framingForObject, coverFramingForObject, adjustedFoV } from './model_framing.mjs';
// Host/lifecycle adapter only. Rendering, loaders, materials and controls are upstream Three.js.
const ENGINE_VERSION = '0.186.1';
let officialDependencies;
const ownedScenes = new WeakMap();

async function dependencies() {
  officialDependencies ??= Promise.all([
    import('./vendor/three/build/three.module.js'),
    import('./vendor/three/examples/jsm/controls/OrbitControls.js'),
    import('./vendor/three/examples/jsm/loaders/GLTFLoader.js'),
    import('./vendor/three/examples/jsm/loaders/PLYLoader.js'),
  ]).then(([THREE, controls, gltf, ply]) => ({
    THREE, OrbitControls: controls.OrbitControls, GLTFLoader: gltf.GLTFLoader, PLYLoader: ply.PLYLoader,
  }));
  return officialDependencies;
}

export async function create(canvas, options = {}) {
  if (!canvas || typeof canvas.getContext !== 'function') throw new Error('A canvas is required');
  const context = canvas.getContext('webgl2', { antialias: true, alpha: false });
  if (!context) throw new Error('WebGL2 is unavailable on this device');
  return createViewerWithDependencies(canvas, { ...options, context }, await dependencies());
}

function materialsOf(node) {
  return Array.isArray(node.material) ? node.material : node.material ? [node.material] : [];
}

function texturesOf(material) {
  return Object.values(material).filter(value => value?.isTexture);
}

function disposeObject(object) {
  if (!object) return;
  const geometries = new Set(), materials = new Set(), textures = new Set(), skeletons = new Set();
  for (const root of ownedScenes.get(object) ?? [object]) root.traverse(node => {
    if (node.geometry) geometries.add(node.geometry);
    if (node.skeleton) skeletons.add(node.skeleton);
    for (const material of materialsOf(node)) {
      materials.add(material);
      for (const texture of texturesOf(material)) textures.add(texture);
    }
  });
  for (const geometry of geometries) geometry.dispose();
  for (const material of materials) material.dispose();
  const images = new Set();
  for (const texture of textures) {
    texture.dispose();
    if (texture.source?.data && typeof texture.source.data.close === 'function') images.add(texture.source.data);
  }
  for (const image of images) image.close();
  for (const skeleton of skeletons) skeleton.dispose?.();
}

function statistics(object) {
  let meshCount = 0, pointCount = 0, triangleCount = 0, normalMapCount = 0;
  const textures = new Set(), materials = new Set();
  object?.traverse(node => {
    const vertices = node.geometry?.getAttribute('position')?.count ?? 0;
    if (node.isMesh) {
      meshCount++;
      triangleCount += (node.geometry.index?.count ?? vertices) / 3;
    }
    if (node.isPoints) pointCount += vertices;
    for (const material of materialsOf(node)) materials.add(material);
  });
  for (const material of materials) {
    if (material.normalMap) normalMapCount++;
    for (const texture of texturesOf(material)) textures.add(texture);
  }
  const loadedTextureCount = [...textures].filter(texture => {
    const image = texture.image ?? texture.source?.data;
    return image && (image.width > 0 || image.height > 0 || image.data?.length > 0);
  }).length;
  return { meshCount, pointCount, triangleCount, materialCount: materials.size,
    textureCount: textures.size, loadedTextureCount, normalMapCount };
}

                                                                                                      
export function createViewerWithDependencies(canvas, options = {}, api) {
  const { THREE, OrbitControls, GLTFLoader, PLYLoader } = api;
  const renderer = new THREE.WebGLRenderer({ canvas, context: options.context, antialias: true, alpha: false });
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1;
  const scene = new THREE.Scene();
  scene.background = new THREE.Color(0x000000);
  let camera = new THREE.PerspectiveCamera(45, 1, 0.001, 1000);
  camera.position.set(0, 0, 3);
  let controls = new OrbitControls(camera, canvas);
  let controlsUp = {x: camera.up.x, y: camera.up.y, z: camera.up.z};
                                                                                
  controls.enableDamping = false;
  controls.screenSpacePanning = true;
  const ambient = new THREE.AmbientLight(0xffffff, 2);
  const key = new THREE.DirectionalLight(0xffffff, 3);
  key.position.set(3, 5, 4);
  scene.add(ambient, key);
  const gltfLoader = new GLTFLoader(), plyLoader = new PLYLoader();
                                                                   
  plyLoader.setCustomPropertyNameMapping?.({
    pwInitialPointPixels: ['pw_initial_point_pixels'],
  });
  useOfficialTextureLoader(gltfLoader, THREE);
  let current = null, request = 0, disposed = false, paused = false;
  let importAbort = null, importInfo = null;
  let framing = null, userHasInteracted = false, interactionSerial = 0;
  let capturePinhole = null, selectionCamera = null;
  let capturePointSizeAnchor = null;
  const framedFoVDeg = 45;
                                                                             
                                                                                
                                                                          
  const fieldOfViewAspect = 1;
  let width = 1, height = 1, pixelRatio = 1, status = 'empty', error = null, url = null, format = null;

  const makeSurfaceComments = () => createSurfaceComments({THREE, canvas, camera, controls,
    getModel: () => current, getFormat: () => format,
    getOverviewDistance: () => framing?.idealCameraDistance * 1.05,
    emit: (type, detail) => options.onSurfaceEvent?.(type, detail), onFocusPoint: focusPoint});
  let surfaceComments = makeSurfaceComments();
  const selectionPreview = createSelectionPreview({scene, renderer, getModel: () => current});
  const assertActive = () => { if (disposed) throw new Error('Viewer is disposed'); };
  let lastAnimationTime = null, maxFps = 60;
  const render = time => {
    if (disposed) return;
    if (typeof time === 'number' && lastAnimationTime !== null && time - lastAnimationTime < 1000 / maxFps) return;
    const delta = typeof time === 'number' && lastAnimationTime !== null ? Math.max(0, (time - lastAnimationTime) / 1000) : 0;
    if (typeof time === 'number') lastAnimationTime = time;
                                                                             
                                                                               
    if (controls.autoRotate || controls.enableDamping) controls.update(delta);
    renderer.render(scene, camera); surfaceComments.update();
  };
  let background = 0x000000, pointSize = options.pointSize ?? 0.005, autoRotateUntilTouched = false;
  const stopOnTouch = () => {
    interactionSerial++;
    userHasInteracted = true;
    if (autoRotateUntilTouched) { controls.autoRotate = false; autoRotateUntilTouched = false; }
    notify();
  };
  controls.addEventListener?.('start', stopOnTouch);
  const coordinates = vector => ({ x: vector.x, y: vector.y, z: vector.z });
  const getState = () => ({
    engine: 'three', engineVersion: ENGINE_VERSION, engineRevision: THREE.REVISION,
    bundleIdentity: options.bundleIdentity ?? null,
    importInfo, idealAspect: framing?.idealAspect ?? null,
    status, error, url, format, width, height, pixelRatio, paused, disposed, userHasInteracted,
    surfaceCommentMode: surfaceComments.enabled,
    background, pointSize, maxFps, interactionEnabled: controls.enabled !== false,
    autoRotate: controls.autoRotate === true, autoRotateUntilTouched,
    ...statistics(current), camera: { position: coordinates(camera.position), target: coordinates(controls.target),
      near: camera.near, far: camera.far, zoom: camera.zoom },
  });
  const notify = () => {
    try { options.onStateChange?.(getState()); }
    catch (failure) { console.warn('Viewer state observer failed', failure); }
  };

                                                                          
  function applyInitialCamera(targetCamera, nextFraming) {
    const { center, radius, idealCameraDistance } = nextFraming;
    targetCamera.clearViewOffset?.();
    targetCamera.up.set(0, 1, 0);
    targetCamera.zoom = 1;
    targetCamera.fov = adjustedFoV(framedFoVDeg, fieldOfViewAspect, targetCamera.aspect);
    targetCamera.position.copy(center).add(
      new THREE.Vector3(1, Math.SQRT2, 1).normalize().multiplyScalar(idealCameraDistance * 1.05));
    targetCamera.near = Math.max(radius / 10000, 0.000001);
    targetCamera.far = Math.max(radius * 1000, targetCamera.near * 1000);
    targetCamera.lookAt(center);
    targetCamera.updateProjectionMatrix();
    targetCamera.updateMatrixWorld();
  }

                                                                              
                                                                             
  function rebindCameraUp(force = false) {
    if (!force && controlsUp.x === camera.up.x && controlsUp.y === camera.up.y && controlsUp.z === camera.up.z) return;
    const old = controls;
    const position = camera.position.clone(), target = old.target.clone();
    const quaternion = camera.quaternion?.clone();
    const config = {enabled: old.enabled, autoRotate: old.autoRotate,
      autoRotateSpeed: old.autoRotateSpeed, minDistance: old.minDistance, maxDistance: old.maxDistance};
    surfaceComments.dispose();
    old.removeEventListener?.('start', stopOnTouch); old.dispose();
    controls = new OrbitControls(camera, canvas);
    Object.assign(controls, config, {enableDamping: false, screenSpacePanning: true});
    camera.position.copy(position); controls.target.copy(target);
    if (quaternion) camera.quaternion.copy(quaternion);
    camera.updateMatrixWorld?.();
    controls.addEventListener?.('start', stopOnTouch);
    controlsUp = {x: camera.up.x, y: camera.up.y, z: camera.up.z};
    surfaceComments = makeSurfaceComments();
  }

                                                                         
                                                                            
                                                                              
  function applyCapturePinhole(targetCamera, input) {
    targetCamera.fov = 2 * Math.atan(height / (2 * input.focalPixels)) * 180 / Math.PI;
    targetCamera.aspect = width / height;
    targetCamera.setViewOffset(width, height,
      input.centeredFocus ? 0 : width / 2 - input.principalX,
      input.centeredFocus ? 0 : height / 2 - input.principalY, width, height);
    targetCamera.updateProjectionMatrix();
  }
  function applyCaptureCamera(input, nextFraming) {
    for (const key of ['position', 'target', 'up']) {
      if (!Array.isArray(input[key]) || input[key].length !== 3 ||
          !input[key].every(Number.isFinite)) throw new Error('Invalid capture camera vector');
    }
    if (!(input.focalPixels > 0) || !Number.isFinite(input.focalPixels) ||
        !Number.isFinite(input.principalX) || !Number.isFinite(input.principalY)) {
      throw new Error('Invalid capture camera pinhole');
    }
                                                                                   
    fit(nextFraming);
    camera.position.fromArray(input.position); camera.up.fromArray(input.up);
    controls.target.fromArray(input.target);
    const distance = camera.position.distanceTo(controls.target);
    if (!(distance > 0)) throw new Error('Invalid capture camera distance');
                                                                              
                                                                             
    const boundsDistance = nextFraming
      ? camera.position.distanceTo(nextFraming.center) + nextFraming.radius
      : distance;
    camera.near = Math.min(camera.near, distance / 2);
    camera.far = Math.max(camera.far, boundsDistance * 1.05, distance * 1.05);
    controls.minDistance = camera.near * 2;
    controls.maxDistance = Math.max(camera.far / 2, distance * 1.05);
    camera.lookAt(controls.target);
    rebindCameraUp();
    applyCapturePinhole(camera, input);
    camera.updateMatrixWorld();
    controls.saveState();
  }

  function fit(nextFraming) {
    if (!nextFraming) return;
    const { center, radius, idealCameraDistance } = nextFraming;
    controls.target.copy(center);
    applyInitialCamera(camera, nextFraming);
    rebindCameraUp();
    controls.minDistance = camera.near * 2;
    controls.maxDistance = camera.far / 2;
    controls.update();
    controls.saveState();
  }

  function focusPoint(point) {
    if (disposed || selectionCamera || !current || controls.enabled === false) return;
                                                                              
                                                                            
    const offset = camera.position.clone().sub(controls.target);
    controls.target.copy(point);
    camera.position.copy(point).add(offset);
    if (capturePinhole) {
      capturePinhole = {...capturePinhole, centeredFocus: true};
      applyCapturePinhole(camera, capturePinhole);
    }
    controls.autoRotate = false; autoRotateUntilTouched = false;
    userHasInteracted = true; interactionSerial++;
    controls.update(); camera.updateMatrixWorld(); render(); notify();
  }

  function anchorCapturePointSizes(points, previousAnchor) {
    if (!points.isPoints) return null;
    const geometry = points.geometry;
    const initialPixels = geometry.getAttribute('pwInitialPointPixels');
    if (!initialPixels) return null;
    // Upstream PLYLoader emits NaNs for an absent mapped property.
    if (initialPixels.array.every(Number.isNaN)) return null;
    const positions = geometry.getAttribute('position');
    if (initialPixels.count !== positions.count) throw new Error('Invalid point diameter count');
    camera.updateMatrixWorld(true);
    points.updateMatrixWorld(true);
    const viewPosition = new THREE.Vector3();
    const sizes = new Float32Array(positions.count);
    const anchor = previousAnchor ?? {
      viewMatrix: camera.matrixWorldInverse.clone(),
      physicalScale: height * pixelRatio / 2,
      materialSize: points.material.size,
    };
    for (let index = 0; index < positions.count; index++) {
      const pixels = initialPixels.getX(index);
      if (!Number.isFinite(pixels) || pixels < 0) throw new Error('Invalid point diameter');
      viewPosition.fromBufferAttribute(positions, index)
        .applyMatrix4(points.matrixWorld).applyMatrix4(anchor.viewMatrix);
      const depth = -viewPosition.z;
                                                                        
      sizes[index] = depth > 0 ? pixels * depth / anchor.physicalScale : 0;
    }
    geometry.setAttribute('pwPointSize', new THREE.Float32BufferAttribute(sizes, 1));
    const material = points.material;
    const initialMaterialSize = anchor.materialSize;
    material.onBeforeCompile = shader => {
                                                                            
                                                                               
      shader.uniforms.pwInitialMaterialSize = {value: initialMaterialSize};
      shader.vertexShader = 'attribute float pwPointSize;\nuniform float pwInitialMaterialSize;\n' +
        shader.vertexShader.replace('gl_PointSize = size;',
          'gl_PointSize = pwPointSize * ( size / pwInitialMaterialSize );');
    };
    material.customProgramCacheKey = () => 'pw-capture-point-size-v1';
    material.needsUpdate = true;
    return anchor;
  }

  async function load(input) {
    assertActive();
    surfaceComments.clear();
    const ticket = ++request;
    const interactionAtRequest = interactionSerial;
    importAbort?.abort();
    const abort = new AbortController();
    importAbort = abort;
    status = 'loading'; error = null; notify();
    let candidate = null, replacementStarted = false, previous = null, cameraBefore = null;
    let candidateImportInfo = null;
    try {
      if (!input?.url || !['glb', 'ply', 'skp'].includes(input.format)) throw new Error('Unsupported model format; use glb, ply or skp');
      if (input.format === 'glb' || input.format === 'skp') {
        let gltf;
        if (input.format === 'skp') {
          const converter = options.convertSkpUrl ?? (await import('./skp_import.mjs')).convertSkpUrl;
          const converted = await converter(input.url, { signal: abort.signal });
          candidateImportInfo = { ...converted.info, fidelity: 'surface-preview-not-lossless-certified' };
          gltf = await gltfLoader.parseAsync(converted.bytes, '');
        } else gltf = await gltfLoader.loadAsync(input.url);
        candidate = gltf.scene;
        if (!candidate) throw new Error('GLB has no default scene');
        ownedScenes.set(candidate, gltf.scenes?.length ? gltf.scenes : [candidate]);
      } else {
        const geometry = await plyLoader.loadAsync(input.url);
        const colored = !!geometry.getAttribute('color');
        if (geometry.index) {
          if (!geometry.getAttribute('normal')) geometry.computeVertexNormals();
          candidate = new THREE.Mesh(geometry, new THREE.MeshStandardMaterial({
            color: 0xffffff, vertexColors: colored, side: THREE.DoubleSide,
          }));
        } else {
                                                                                         
          const sprite = canvas.ownerDocument.createElement('canvas');
          sprite.width = sprite.height = 16;
          const spriteContext = sprite.getContext('2d');
          spriteContext.fillStyle = '#ffffff';
          spriteContext.beginPath();
          spriteContext.arc(8, 8, 7, 0, Math.PI * 2);
          spriteContext.fill();
          candidate = new THREE.Points(geometry, new THREE.PointsMaterial({
            color: 0xffffff, vertexColors: colored, size: pointSize, sizeAttenuation: true,
            map: new THREE.CanvasTexture(sprite), transparent: true, alphaTest: 0.01,
          }));
        }
      }
      if (disposed || ticket !== request) {
        disposeObject(candidate);
        return { status: 'superseded' };
      }
      // The old model remains until the complete upstream load succeeds.
      const candidateFraming = framingForObject(candidate, framedFoVDeg);
      previous = current;
      cameraBefore = { position: new THREE.Vector3().copy(camera.position),
        target: new THREE.Vector3().copy(controls.target), quaternion: camera.quaternion?.clone(),
        up: camera.up.clone(), view: camera.view ? {...camera.view} : null,
        fov: camera.fov, near: camera.near, far: camera.far, minDistance: controls.minDistance, maxDistance: controls.maxDistance };
      replacementStarted = true;
                                                                          
                                                                                 
      const keepNewInteraction = input.initialCaptureCamera != null &&
        interactionSerial !== interactionAtRequest;
      const shouldFit = (!previous || !input.preserveCamera) && !keepNewInteraction;
      const nextCapture = !input.preserveCamera && !keepNewInteraction
        ? input.initialCaptureCamera : null;
      if (nextCapture) applyCaptureCamera(nextCapture, candidateFraming);
      else if (shouldFit) fit(candidateFraming);
      const nextPointSizeAnchor = anchorCapturePointSizes(candidate, capturePointSizeAnchor);
      scene.add(candidate);
      if (previous) scene.remove(previous);
      render();
      current = candidate; url = input.url; format = input.format; status = 'ready'; error = null;
      capturePointSizeAnchor = nextPointSizeAnchor;
      importInfo = candidateImportInfo;
      framing = candidateFraming;
      if (nextCapture) capturePinhole = nextCapture;
      else if (shouldFit) capturePinhole = null;
      if (shouldFit) userHasInteracted = false;
      replacementStarted = false;
      disposeObject(previous);
      notify();
      return { ...getState(), status: 'loaded' };
    } catch (failure) {
      if (replacementStarted) {
        scene.remove(candidate);
        if (previous) scene.add(previous);
        camera.position.copy(cameraBefore.position);
        controls.target.copy(cameraBefore.target);
        if (cameraBefore.quaternion) camera.quaternion.copy(cameraBefore.quaternion);
        camera.up.copy(cameraBefore.up); camera.view = cameraBefore.view;
        rebindCameraUp();
        camera.fov = cameraBefore.fov;
        camera.near = cameraBefore.near; camera.far = cameraBefore.far;
        controls.minDistance = cameraBefore.minDistance; controls.maxDistance = cameraBefore.maxDistance;
        camera.updateProjectionMatrix();
        try { render(); } catch {                                                                }
      }
      if (candidate && candidate !== current) disposeObject(candidate);
      if (disposed || ticket !== request) return { status: 'superseded' };
      status = 'error'; error = failure instanceof Error ? failure.message : String(failure);
      notify(); throw failure;
    } finally {
      if (importAbort === abort) importAbort = null;
    }
  }

  function configure(config = {}) {
    assertActive();
    for (const key of ['interactive', 'autoRotate', 'autoRotateUntilTouched']) {
      if (key in config && typeof config[key] !== 'boolean') throw new Error(`Invalid ${key}`);
    }
    if ('maxFps' in config && (!Number.isFinite(config.maxFps) || config.maxFps < 1 || config.maxFps > 60)) throw new Error('Invalid frame budget');
    if ('background' in config && (!Number.isInteger(config.background) || config.background < 0 || config.background > 0xffffff)) throw new Error('Invalid background');
    if ('pointSize' in config && (!Number.isFinite(config.pointSize) || config.pointSize <= 0)) throw new Error('Invalid point size');
    if ('autoRotateSpeed' in config && !Number.isFinite(config.autoRotateSpeed)) throw new Error('Invalid rotation speed');
    if ('maxFps' in config) maxFps = config.maxFps;
    if ('interactive' in config) controls.enabled = config.interactive;
    if ('autoRotate' in config) controls.autoRotate = config.autoRotate;
    if ('autoRotateUntilTouched' in config) autoRotateUntilTouched = config.autoRotateUntilTouched;
    if (surfaceComments.enabled) { controls.autoRotate = false; autoRotateUntilTouched = false; }
    if ('autoRotateSpeed' in config) controls.autoRotateSpeed = config.autoRotateSpeed;
    if ('background' in config) { background = config.background; scene.background = new THREE.Color(background); }
    if ('pointSize' in config) {
      pointSize = config.pointSize;
      current?.traverse(node => { if (node.isPoints) for (const material of materialsOf(node)) material.size = pointSize; });
    }
    render(); notify(); return getState();
  }
  function resetCamera() {
    assertActive();
    const resetCapture = capturePinhole != null;
    capturePinhole = null;
    if (resetCapture && framing) fit(framing);
    else controls.reset?.();
    userHasInteracted = false; camera.clearViewOffset?.();
    if (framing) camera.fov = adjustedFoV(framedFoVDeg, fieldOfViewAspect, camera.aspect);
    camera.updateProjectionMatrix(); render(); notify(); return getState();
  }

  function resize(nextWidth = canvas.clientWidth, nextHeight = canvas.clientHeight, nextPixelRatio = options.pixelRatio ?? globalThis.devicePixelRatio ?? 1) {
    assertActive();
                                                                          
                                                                             
    if (!(Number(nextWidth) > 1 && Number(nextHeight) > 1)) return;
    width = Math.max(1, Math.floor(Number(nextWidth) || 1));
    height = Math.max(1, Math.floor(Number(nextHeight) || 1));
    pixelRatio = Math.max(0.25, Math.min(3, Number(nextPixelRatio) || 1));
    renderer.setPixelRatio(pixelRatio);
    renderer.setSize(width, height, false);
    camera.aspect = width / height;
                                                                                  
                                                                               
    if (capturePinhole) {
      applyCapturePinhole(camera, capturePinhole);
    } else if (framing) {
      camera.fov = adjustedFoV(framedFoVDeg, fieldOfViewAspect, camera.aspect);
    }
    if (selectionCamera) updateSelectionProjection();
    camera.updateProjectionMatrix(); render(); notify();
  }
  function capture(type = 'image/png', quality, canonicalCamera = false) {
    assertActive();
    if (!canonicalCamera) {
      render();
      return renderer.domElement.toDataURL(type, quality);
    }
    if (!current || !framing) throw new Error('Cover requires a loaded source');
                                                                          
                                                                            
                                                                       
    const coverFraming = coverFramingForObject(current);
    if (!coverFraming) throw new Error('Cover requires finite source geometry');
    const halfShortSide = coverFraming.radius * 8 / 6.5;
    const aspect = width / height;
    const halfWidth = halfShortSide * Math.max(1, aspect);
    const halfHeight = halfShortSide * Math.max(1, 1 / aspect);
                                                                             
                                                                                
    const extent = framing.radius + framing.center.distanceTo(coverFraming.center);
    const distance = Math.max(extent * 2, coverFraming.radius * 8);
    const coverCamera = new THREE.OrthographicCamera(
      -halfWidth, halfWidth, halfHeight, -halfHeight,
      Math.max(1e-6, distance - extent * 1.01), distance + extent * 1.01);
    coverCamera.position.copy(coverFraming.center).add(
      new THREE.Vector3(0, Math.SQRT1_2, Math.SQRT1_2).multiplyScalar(distance));
    coverCamera.lookAt(coverFraming.center);
    coverCamera.updateProjectionMatrix();
    coverCamera.updateMatrixWorld();
    try {
      renderer.render(scene, coverCamera);
      return renderer.domElement.toDataURL(type, quality);
    } finally {
                                                                              
                                                               
      renderer.render(scene, camera);
    }
  }
  function pause() { assertActive(); lastAnimationTime = null; paused = true; renderer.setAnimationLoop(null); notify(); }
  function resume() { assertActive(); lastAnimationTime = null; paused = false; renderer.setAnimationLoop(render); notify(); }
  function dispose() {
    if (disposed) return;
    selectionPreview.clear();
    disposed = true; request++; renderer.setAnimationLoop(null);
    importAbort?.abort(); importAbort = null;
    controls.removeEventListener?.('start', stopOnTouch);
    surfaceComments.dispose();
    controls.dispose();
    if (current) scene.remove(current);
    disposeObject(current); current = null; framing = null;
    renderer.dispose(); status = 'disposed'; notify();
  }
  function setSurfaceCommentMode(enabled) {
    assertActive(); surfaceComments.setMode(enabled);
    if (enabled) { autoRotateUntilTouched = false; userHasInteracted = true; }
    render(); notify();
  }
  function setSurfaceCommentMarkers(markers) { assertActive(); surfaceComments.setMarkers(markers); render(); }
  function focusSurfaceComment(anchor) {
    assertActive(); surfaceComments.focus(anchor); autoRotateUntilTouched = false; userHasInteracted = true; render(); notify();
  }
  function updateSelectionProjection() {
    const scale = Math.min(width,height) * 0.5 * 6.5 / (selectionCamera.radius * 8);
    camera.left = -width / (2*scale); camera.right = width / (2*scale);
    camera.top = height / (2*scale); camera.bottom = -height / (2*scale);
    camera.updateProjectionMatrix();
  }
  function beginSelectionEditing(radius) {
    assertActive();
    if (!getSelectionBounds() || !Number.isFinite(radius) || radius <= 0) throw new Error('GLB selection framing unavailable');
    if (selectionCamera) throw new Error('Selection editing already active');
    selectionCamera = {camera, radius, target:controls.target.clone(), capturePinhole,
      autoRotate:controls.autoRotate, autoRotateUntilTouched};
    const original = camera;
    camera = new THREE.OrthographicCamera(-1,1,1,-1,Math.max(radius/10000,1e-6),radius*1000);
    camera.position.copy(original.position); camera.up.copy(original.up); camera.quaternion.copy(original.quaternion);
    capturePinhole = null;
    rebindCameraUp(true); controls.autoRotate = false; autoRotateUntilTouched = false;
    updateSelectionProjection(); render(); notify();
  }
  function endSelectionEditing() {
    if (!selectionCamera) return;
    selectionPreview.clear();
    const saved = selectionCamera; selectionCamera = null;
    camera = saved.camera; capturePinhole = saved.capturePinhole;
    controls.target.copy(saved.target); rebindCameraUp(true);
    controls.autoRotate = saved.autoRotate; autoRotateUntilTouched = saved.autoRotateUntilTouched;
    camera.updateMatrixWorld(); render(); notify();
  }
  function getCameraFraming() {
    assertActive();
    if (selectionCamera || !camera.isPerspectiveCamera || !current) throw new Error('Perspective camera unavailable');
    return {projection:'perspective', position:camera.position.toArray(), target:controls.target.toArray(),
      up:camera.up.toArray(), fov_y_degrees:camera.fov, aspect:camera.aspect};
  }
  function getSelectionView() {
    assertActive(); camera.updateMatrixWorld();
    const e = camera.matrixWorldInverse.elements;
    return [-e[0],-e[4],-e[8],e[1],e[5],e[9],-e[2],-e[6],-e[10]];
  }
  function setSelectionView(rotation, target) {
    assertActive();
    if (!getSelectionBounds() || !Array.isArray(rotation) || rotation.length !== 9 ||
        !rotation.every(Number.isFinite) || !Array.isArray(target) || target.length !== 3 || !target.every(Number.isFinite))
      throw new Error('Invalid selection camera');
                                                                                   
    const distance = selectionCamera ? selectionCamera.radius*8 : camera.position.distanceTo(controls.target);
    controls.target.fromArray(target);
    camera.position.copy(controls.target).add(new THREE.Vector3(-rotation[6],-rotation[7],-rotation[8]).multiplyScalar(distance));
    camera.up.set(rotation[3],rotation[4],rotation[5]);
    camera.lookAt(controls.target); camera.updateMatrixWorld();
    rebindCameraUp();
    render(); notify();
  }
  let selectionBoundsModel = null, selectionBounds = null;
  function getSelectionBounds() {
    assertActive();
    if (selectionBoundsModel !== current) {
      selectionBoundsModel = current; selectionBounds = sourceWorldBounds(current);
    }
    return format === 'glb' && status === 'ready' ? selectionBounds : null;
  }
  function setSelectionPreview(box, decorations = true) {
    assertActive();
    if (box != null && !getSelectionBounds()) throw new Error('GLB source bounds unavailable');
    selectionPreview.set(box, decorations); render();
  }
  resize(); resume();
  return { getCameraFraming, beginSelectionEditing, endSelectionEditing, getSelectionView, setSelectionView, getSelectionBounds, setSelectionPreview, setSurfaceCommentMode, setSurfaceCommentMarkers, focusSurfaceComment, load, configure, resetCamera, resize, capture, pause, resume, dispose, getState };
}

if (typeof window !== 'undefined') window.PWSharedViewer = Object.freeze({ create, engineVersion: ENGINE_VERSION });
