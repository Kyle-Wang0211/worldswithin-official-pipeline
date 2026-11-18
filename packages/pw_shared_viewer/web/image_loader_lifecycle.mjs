                                                                                
                                                                             
                                                                             
export function useOfficialTextureLoader(gltfLoader, THREE) {
  gltfLoader.register(parser => ({
    name: 'PW_OFFICIAL_TEXTURE_LOADER_LIFECYCLE',
    beforeRoot() {
      const loader = new THREE.TextureLoader(parser.options.manager);
      loader.setCrossOrigin(parser.options.crossOrigin);
      loader.setRequestHeader(parser.options.requestHeader);
      parser.textureLoader = loader;
    },
  }));
}
