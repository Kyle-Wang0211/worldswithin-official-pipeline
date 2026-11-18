struct PwFixedLoaderSpec{std::vector<TensorShape> inputs;TensorShape output;size_t data_inputs;bool has_bias;int64_t n,c,ih,iw,o,oh,ow,ky,kx,sy,sx,dy,dx,py,px;bool fit_a,fit_b,fit_inner;const char* reader;const char* root;};
static const std::vector<PwFixedLoaderSpec>& PwFixedLoaderSpecs(){static const std::vector<PwFixedLoaderSpec> specs={
{{TensorShape{1,16,576,768},TensorShape{1,15,576,768},TensorShape{1,1,576,768},TensorShape{8,32,7,7},TensorShape{8}},TensorShape{1,8,576,768},3,true,1,32,576,768,8,576,768,7,7,1,1,1,1,3,3,true,false,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*6635520+(i1)*442368+(i2)*768+(i3)*1));}
fn readT3(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL2(((i0)*442368+(i1)*442368+(i2)*768+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<16){return readT1(i0,(i1-0),i2,i3);}if(i1<31){return readT2(i0,(i1-16),i2,i3);}if(i1<32){return readT3(i0,(i1-31),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{1,31,288,384},TensorShape{1,1,288,384},TensorShape{16,64,7,7},TensorShape{16}},TensorShape{1,16,288,384},3,true,1,64,288,384,16,288,384,7,7,1,1,1,1,3,3,true,false,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*3428352+(i1)*110592+(i2)*384+(i3)*1));}
fn readT3(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL2(((i0)*110592+(i1)*110592+(i2)*384+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<63){return readT2(i0,(i1-32),i2,i3);}if(i1<64){return readT3(i0,(i1-63),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,4,48,144,192},TensorShape{8,12,3,3}},TensorShape{48,8,144,192},1,false,48,12,144,192,8,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{let sourceDepth=i0+i1/4-1;if(sourceDepth<0||sourceDepth>=48){return 0.0;}return readL0(((i1%4)*48+sourceDepth)*27648+i2*192+i3);}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{1,15,576,768},TensorShape{1,1,576,768},TensorShape{8,32,7,7},TensorShape{8}},TensorShape{1,8,576,768},3,true,1,32,576,768,8,576,768,7,7,1,1,1,1,3,3,true,false,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*6635520+(i1)*442368+(i2)*768+(i3)*1));}
fn readT3(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL2(((i0)*442368+(i1)*442368+(i2)*768+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<16){return readT1(i0,(i1-0),i2,i3);}if(i1<31){return readT2(i0,(i1-16),i2,i3);}if(i1<32){return readT3(i0,(i1-31),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{1,15,576,768},TensorShape{1,1,576,768},TensorShape{8,32,7,7},TensorShape{8}},TensorShape{1,8,576,768},3,true,1,32,576,768,8,576,768,7,7,1,1,1,1,3,3,true,false,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*6635520+(i1)*442368+(i2)*768+(i3)*1));}
fn readT3(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL2(((i0)*442368+(i1)*442368+(i2)*768+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<16){return readT1(i0,(i1-0),i2,i3);}if(i1<31){return readT2(i0,(i1-16),i2,i3);}if(i1<32){return readT3(i0,(i1-31),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{1,31,288,384},TensorShape{1,1,288,384},TensorShape{16,64,7,7},TensorShape{16}},TensorShape{1,16,288,384},3,true,1,64,288,384,16,288,384,7,7,1,1,1,1,3,3,true,false,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*3428352+(i1)*110592+(i2)*384+(i3)*1));}
fn readT3(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL2(((i0)*110592+(i1)*110592+(i2)*384+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<63){return readT2(i0,(i1-32),i2,i3);}if(i1<64){return readT3(i0,(i1-63),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{1,31,288,384},TensorShape{1,1,288,384},TensorShape{16,64,7,7},TensorShape{16}},TensorShape{1,16,288,384},3,true,1,64,288,384,16,288,384,7,7,1,1,1,1,3,3,true,false,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*3428352+(i1)*110592+(i2)*384+(i3)*1));}
fn readT3(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL2(((i0)*110592+(i1)*110592+(i2)*384+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<63){return readT2(i0,(i1-32),i2,i3);}if(i1<64){return readT3(i0,(i1-63),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{36,16,3,3},TensorShape{36}},TensorShape{1,36,576,768},1,true,1,16,576,768,36,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,48,144,192},TensorShape{8,24,3,3}},TensorShape{48,8,144,192},1,false,48,24,144,192,8,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT5(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readL0(((i0)*10616832+(i1)*1327104+(i2)*27648+(i3)*192+(i4)*1));}
fn readT4(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{if((i0-0)<0 || (i0-0)>=1 || (i1-0)<0 || (i1-0)>=8 || (i2-1)<0 || (i2-1)>=48 || (i3-0)<0 || (i3-0)>=144 || (i4-0)<0 || (i4-0)>=192){return 0.0;}return readT5((i0-0),(i1-0),(i2-1),(i3-0),(i4-0));}
fn readT3(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT4(i0,i1,(i2*1+0),i3,i4);}
fn readT6(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT4(i0,i1,(i2*1+1),i3,i4);}
fn readT7(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT4(i0,i1,(i2*1+2),i3,i4);}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{if(i1<8){return readT3(i0,(i1-0),i2,i3,i4);}if(i1<16){return readT6(i0,(i1-8),i2,i3,i4);}if(i1<24){return readT7(i0,(i1-16),i2,i3,i4);}return 0.0; // unreachable under strict output shape
}
fn readT1(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT2(i0,i2,i1,i3,i4);}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readT1(((((i0)*663552+(i1)*27648+(i2)*192+(i3)*1))/31850496)%1,((((i0)*663552+(i1)*27648+(i2)*192+(i3)*1))/663552)%48,((((i0)*663552+(i1)*27648+(i2)*192+(i3)*1))/27648)%24,((((i0)*663552+(i1)*27648+(i2)*192+(i3)*1))/192)%144,((((i0)*663552+(i1)*27648+(i2)*192+(i3)*1))/1)%192);}
)PWLF","readT0"},
{{TensorShape{1,4,48,144,192},TensorShape{8,12,3,3}},TensorShape{48,8,144,192},1,false,48,12,144,192,8,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{let sourceDepth=i0+i1/4-1;if(sourceDepth<0||sourceDepth>=48){return 0.0;}return readL0(((i1%4)*48+sourceDepth)*27648+i2*192+i3);}
)PWLF","readT0"},
{{TensorShape{1,4,4,576,768},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,16,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readL0(((i0)*7077888+(i1)*1769472+(i2)*442368+(i3)*768+(i4)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readT1(((((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1))/7077888)%1,((((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1))/1769472)%4,((((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1))/442368)%4,((((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1))/768)%576,((((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1))/1)%768);}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,16,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,4,48,144,192},TensorShape{8,12,3,3}},TensorShape{48,8,144,192},1,false,48,12,144,192,8,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{let sourceDepth=i0+i1/4-1;if(sourceDepth<0||sourceDepth>=48){return 0.0;}return readL0(((i1%4)*48+sourceDepth)*27648+i2*192+i3);}
)PWLF","readT0"},
{{TensorShape{1,4,48,144,192},TensorShape{8,12,3,3}},TensorShape{48,8,144,192},1,false,48,12,144,192,8,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{let sourceDepth=i0+i1/4-1;if(sourceDepth<0||sourceDepth>=48){return 0.0;}return readL0(((i1%4)*48+sourceDepth)*27648+i2*192+i3);}
)PWLF","readT0"},
{{TensorShape{1,4,48,144,192},TensorShape{8,12,3,3}},TensorShape{48,8,144,192},1,false,48,12,144,192,8,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT5(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readL0(((i0)*5308416+(i1)*1327104+(i2)*27648+(i3)*192+(i4)*1));}
fn readT4(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{if((i0-0)<0 || (i0-0)>=1 || (i1-0)<0 || (i1-0)>=4 || (i2-1)<0 || (i2-1)>=48 || (i3-0)<0 || (i3-0)>=144 || (i4-0)<0 || (i4-0)>=192){return 0.0;}return readT5((i0-0),(i1-0),(i2-1),(i3-0),(i4-0));}
fn readT3(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT4(i0,i1,(i2*1+0),i3,i4);}
fn readT6(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT4(i0,i1,(i2*1+1),i3,i4);}
fn readT7(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT4(i0,i1,(i2*1+2),i3,i4);}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{if(i1<4){return readT3(i0,(i1-0),i2,i3,i4);}if(i1<8){return readT6(i0,(i1-4),i2,i3,i4);}if(i1<12){return readT7(i0,(i1-8),i2,i3,i4);}return 0.0; // unreachable under strict output shape
}
fn readT1(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT2(i0,i2,i1,i3,i4);}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readT1(((((i0)*331776+(i1)*27648+(i2)*192+(i3)*1))/15925248)%1,((((i0)*331776+(i1)*27648+(i2)*192+(i3)*1))/331776)%48,((((i0)*331776+(i1)*27648+(i2)*192+(i3)*1))/27648)%12,((((i0)*331776+(i1)*27648+(i2)*192+(i3)*1))/192)%144,((((i0)*331776+(i1)*27648+(i2)*192+(i3)*1))/1)%192);}
)PWLF","readT0"},
{{TensorShape{1,4,48,144,192},TensorShape{8,12,3,3}},TensorShape{48,8,144,192},1,false,48,12,144,192,8,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{let sourceDepth=i0+i1/4-1;if(sourceDepth<0||sourceDepth>=48){return 0.0;}return readL0(((i1%4)*48+sourceDepth)*27648+i2*192+i3);}
)PWLF","readT0"},
{{TensorShape{1,4,48,144,192},TensorShape{8,12,3,3}},TensorShape{48,8,144,192},1,false,48,12,144,192,8,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{let sourceDepth=i0+i1/4-1;if(sourceDepth<0||sourceDepth>=48){return 0.0;}return readL0(((i1%4)*48+sourceDepth)*27648+i2*192+i3);}
)PWLF","readT0"},
{{TensorShape{1,4,48,144,192},TensorShape{8,12,3,3}},TensorShape{48,8,144,192},1,false,48,12,144,192,8,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{let sourceDepth=i0+i1/4-1;if(sourceDepth<0||sourceDepth>=48){return 0.0;}return readL0(((i1%4)*48+sourceDepth)*27648+i2*192+i3);}
)PWLF","readT0"},
{{TensorShape{1,4,48,144,192},TensorShape{8,12,3,3}},TensorShape{48,8,144,192},1,false,48,12,144,192,8,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{let sourceDepth=i0+i1/4-1;if(sourceDepth<0||sourceDepth>=48){return 0.0;}return readL0(((i1%4)*48+sourceDepth)*27648+i2*192+i3);}
)PWLF","readT0"},
{{TensorShape{1,4,48,144,192},TensorShape{8,12,3,3}},TensorShape{48,8,144,192},1,false,48,12,144,192,8,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{let sourceDepth=i0+i1/4-1;if(sourceDepth<0||sourceDepth>=48){return 0.0;}return readL0(((i1%4)*48+sourceDepth)*27648+i2*192+i3);}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,16,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{64,16,3,3},TensorShape{64}},TensorShape{1,64,576,768},1,true,1,16,576,768,64,576,768,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{64,32,3,3},TensorShape{64}},TensorShape{1,64,288,384},1,true,1,32,288,384,64,288,384,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{1,8,576,768},TensorShape{8,16,3,3},TensorShape{8}},TensorShape{1,8,576,768},2,true,1,16,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<8){return readT1(i0,(i1-0),i2,i3);}if(i1<16){return readT2(i0,(i1-8),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{8,16,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,16,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{1,8,576,768},TensorShape{8,16,3,3},TensorShape{8}},TensorShape{1,8,576,768},2,true,1,16,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<8){return readT1(i0,(i1-0),i2,i3);}if(i1<16){return readT2(i0,(i1-8),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,32,288,384,32,288,384,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{8,16,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,16,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{64,32,3,3},TensorShape{64}},TensorShape{1,64,288,384},1,true,1,32,288,384,64,288,384,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,4,4,576,768},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,16,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readL0(((i0)*7077888+(i1)*1769472+(i2)*442368+(i3)*768+(i4)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readT1(((((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1))/7077888)%1,((((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1))/1769472)%4,((((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1))/442368)%4,((((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1))/768)%576,((((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1))/1)%768);}
)PWLF","readT0"},
{{TensorShape{1,4,4,576,768},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,16,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readL0(((i0)*7077888+(i1)*1769472+(i2)*442368+(i3)*768+(i4)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readT1(((((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1))/7077888)%1,((((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1))/1769472)%4,((((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1))/442368)%4,((((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1))/768)%576,((((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1))/1)%768);}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,8,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,16,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,24,72,96},TensorShape{16,48,3,3}},TensorShape{24,16,72,96},1,false,24,48,72,96,16,72,96,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT5(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readL0(((i0)*2654208+(i1)*165888+(i2)*6912+(i3)*96+(i4)*1));}
fn readT4(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{if((i0-0)<0 || (i0-0)>=1 || (i1-0)<0 || (i1-0)>=16 || (i2-1)<0 || (i2-1)>=24 || (i3-0)<0 || (i3-0)>=72 || (i4-0)<0 || (i4-0)>=96){return 0.0;}return readT5((i0-0),(i1-0),(i2-1),(i3-0),(i4-0));}
fn readT3(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT4(i0,i1,(i2*1+0),i3,i4);}
fn readT6(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT4(i0,i1,(i2*1+1),i3,i4);}
fn readT7(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT4(i0,i1,(i2*1+2),i3,i4);}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{if(i1<16){return readT3(i0,(i1-0),i2,i3,i4);}if(i1<32){return readT6(i0,(i1-16),i2,i3,i4);}if(i1<48){return readT7(i0,(i1-32),i2,i3,i4);}return 0.0; // unreachable under strict output shape
}
fn readT1(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT2(i0,i2,i1,i3,i4);}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readT1(((((i0)*331776+(i1)*6912+(i2)*96+(i3)*1))/7962624)%1,((((i0)*331776+(i1)*6912+(i2)*96+(i3)*1))/331776)%24,((((i0)*331776+(i1)*6912+(i2)*96+(i3)*1))/6912)%48,((((i0)*331776+(i1)*6912+(i2)*96+(i3)*1))/96)%72,((((i0)*331776+(i1)*6912+(i2)*96+(i3)*1))/1)%96);}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{1,8,576,768},TensorShape{8,16,3,3},TensorShape{8}},TensorShape{1,8,576,768},2,true,1,16,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<8){return readT1(i0,(i1-0),i2,i3);}if(i1<16){return readT2(i0,(i1-8),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,16,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,16,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,16,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,16,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,16,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,16,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{1,8,576,768},TensorShape{8,16,3,3},TensorShape{8}},TensorShape{1,8,576,768},2,true,1,16,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<8){return readT1(i0,(i1-0),i2,i3);}if(i1<16){return readT2(i0,(i1-8),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{1,8,576,768},TensorShape{8,16,3,3},TensorShape{8}},TensorShape{1,8,576,768},2,true,1,16,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<8){return readT1(i0,(i1-0),i2,i3);}if(i1<16){return readT2(i0,(i1-8),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{1,8,576,768},TensorShape{8,16,3,3},TensorShape{8}},TensorShape{1,8,576,768},2,true,1,16,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<8){return readT1(i0,(i1-0),i2,i3);}if(i1<16){return readT2(i0,(i1-8),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{8,16,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,16,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,4,576,768},TensorShape{16,4,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,4,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,8,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{1,16,288,384},TensorShape{16,32,3,3},TensorShape{16}},TensorShape{1,16,288,384},2,true,1,32,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<16){return readT1(i0,(i1-0),i2,i3);}if(i1<32){return readT2(i0,(i1-16),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,32,288,384,32,288,384,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,8,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,8,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,8,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,8,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,8,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,8,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{1,16,288,384},TensorShape{16,32,3,3},TensorShape{16}},TensorShape{1,16,288,384},2,true,1,32,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<16){return readT1(i0,(i1-0),i2,i3);}if(i1<32){return readT2(i0,(i1-16),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{16,32,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,32,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,32,288,384,32,288,384,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,1152,1536},TensorShape{16,8,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,8,1152,1536,16,576,768,3,3,2,2,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*14155776+(i1)*1769472+(i2)*1536+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,1152,1536},TensorShape{16,8,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,8,1152,1536,16,576,768,3,3,2,2,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*14155776+(i1)*1769472+(i2)*1536+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,8,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,8,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,8,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,8,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,8,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,8,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,576,768},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,576,768},1,true,1,8,576,768,8,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{16,32,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,32,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{1,16,288,384},TensorShape{16,32,3,3},TensorShape{16}},TensorShape{1,16,288,384},2,true,1,32,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<16){return readT1(i0,(i1-0),i2,i3);}if(i1<32){return readT2(i0,(i1-16),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,4,4,288,384},TensorShape{32,16,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,16,288,384,32,288,384,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readL0(((i0)*1769472+(i1)*442368+(i2)*110592+(i3)*384+(i4)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readT1(((((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1))/1769472)%1,((((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1))/442368)%4,((((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1))/110592)%4,((((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1))/384)%288,((((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1))/1)%384);}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{16,32,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,32,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{1,16,288,384},TensorShape{16,32,3,3},TensorShape{16}},TensorShape{1,16,288,384},2,true,1,32,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<16){return readT1(i0,(i1-0),i2,i3);}if(i1<32){return readT2(i0,(i1-16),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,32,288,384,32,288,384,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,32,288,384,32,288,384,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,20,576,768},TensorShape{32,20,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,20,576,768,32,288,384,3,3,2,2,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*8847360+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,4,576,768},TensorShape{16,4,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,4,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{1,8,288,384},TensorShape{16,24,3,3},TensorShape{16}},TensorShape{1,16,288,384},2,true,1,24,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*110592+(i2)*384+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<16){return readT1(i0,(i1-0),i2,i3);}if(i1<24){return readT2(i0,(i1-16),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{1,16,288,384},TensorShape{16,32,3,3},TensorShape{16}},TensorShape{1,16,288,384},2,true,1,32,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<16){return readT1(i0,(i1-0),i2,i3);}if(i1<32){return readT2(i0,(i1-16),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{16,32,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,32,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{1,16,288,384},TensorShape{16,32,3,3},TensorShape{16}},TensorShape{1,16,288,384},2,true,1,32,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<16){return readT1(i0,(i1-0),i2,i3);}if(i1<32){return readT2(i0,(i1-16),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,32,288,384,32,288,384,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,32,288,384,32,288,384,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,4,576,768},TensorShape{16,4,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,4,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,32,288,384,32,288,384,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,32,288,384,32,288,384,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{16,32,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,32,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{16,32,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,32,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,4,4,288,384},TensorShape{32,16,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,16,288,384,32,288,384,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readL0(((i0)*1769472+(i1)*442368+(i2)*110592+(i3)*384+(i4)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readT1(((((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1))/1769472)%1,((((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1))/442368)%4,((((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1))/110592)%4,((((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1))/384)%288,((((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1))/1)%384);}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{1,8,288,384},TensorShape{16,24,3,3},TensorShape{16}},TensorShape{1,16,288,384},2,true,1,24,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*110592+(i2)*384+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<16){return readT1(i0,(i1-0),i2,i3);}if(i1<24){return readT2(i0,(i1-16),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{1,8,288,384},TensorShape{16,24,3,3},TensorShape{16}},TensorShape{1,16,288,384},2,true,1,24,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*110592+(i2)*384+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<16){return readT1(i0,(i1-0),i2,i3);}if(i1<24){return readT2(i0,(i1-16),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,48,144,192},TensorShape{48,48,3,3},TensorShape{48}},TensorShape{1,48,144,192},1,true,1,48,144,192,48,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1327104+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,48,144,192},TensorShape{48,48,3,3},TensorShape{48}},TensorShape{1,48,144,192},1,true,1,48,144,192,48,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1327104+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,48,144,192},TensorShape{48,48,3,3},TensorShape{48}},TensorShape{1,48,144,192},1,true,1,48,144,192,48,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1327104+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,16,144,192},TensorShape{32,48,3,3},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,48,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<48){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,4,4,288,384},TensorShape{32,16,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,16,288,384,32,288,384,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readL0(((i0)*1769472+(i1)*442368+(i2)*110592+(i3)*384+(i4)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readT1(((((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1))/1769472)%1,((((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1))/442368)%4,((((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1))/110592)%4,((((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1))/384)%288,((((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1))/1)%384);}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{32,16,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,16,576,768,32,288,384,3,3,2,2,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{32,16,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,16,576,768,32,288,384,3,3,2,2,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*7077888+(i1)*442368+(i2)*768+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,16,144,192},TensorShape{32,48,3,3},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,48,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<48){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,288,384},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,288,384},1,true,1,16,288,384,16,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1769472+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,12,36,48},TensorShape{32,96,3,3}},TensorShape{12,32,36,48},1,false,12,96,36,48,32,36,48,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT5(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readL0(((i0)*663552+(i1)*20736+(i2)*1728+(i3)*48+(i4)*1));}
fn readT4(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{if((i0-0)<0 || (i0-0)>=1 || (i1-0)<0 || (i1-0)>=32 || (i2-1)<0 || (i2-1)>=12 || (i3-0)<0 || (i3-0)>=36 || (i4-0)<0 || (i4-0)>=48){return 0.0;}return readT5((i0-0),(i1-0),(i2-1),(i3-0),(i4-0));}
fn readT3(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT4(i0,i1,(i2*1+0),i3,i4);}
fn readT6(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT4(i0,i1,(i2*1+1),i3,i4);}
fn readT7(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT4(i0,i1,(i2*1+2),i3,i4);}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{if(i1<32){return readT3(i0,(i1-0),i2,i3,i4);}if(i1<64){return readT6(i0,(i1-32),i2,i3,i4);}if(i1<96){return readT7(i0,(i1-64),i2,i3,i4);}return 0.0; // unreachable under strict output shape
}
fn readT1(i0:i32,i1:i32,i2:i32,i3:i32,i4:i32)->f32{return readT2(i0,i2,i1,i3,i4);}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readT1(((((i0)*165888+(i1)*1728+(i2)*48+(i3)*1))/1990656)%1,((((i0)*165888+(i1)*1728+(i2)*48+(i3)*1))/165888)%12,((((i0)*165888+(i1)*1728+(i2)*48+(i3)*1))/1728)%96,((((i0)*165888+(i1)*1728+(i2)*48+(i3)*1))/48)%36,((((i0)*165888+(i1)*1728+(i2)*48+(i3)*1))/1)%48);}
)PWLF","readT0"},
{{TensorShape{1,8,288,384},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,288,384},1,true,1,8,288,384,8,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,16,144,192},TensorShape{32,48,3,3},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,48,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<48){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,8,288,384},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,288,384},1,true,1,8,288,384,8,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,1,5},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,1,5,1,1,1,1,0,2,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{64,32,3,3},TensorShape{64}},TensorShape{1,64,144,192},1,true,1,32,144,192,64,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,16,144,192},TensorShape{32,48,3,3},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,48,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<48){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,1,5},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,1,5,1,1,1,1,0,2,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,5,1},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,5,1,1,1,1,1,2,0,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,1,5},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,1,5,1,1,1,1,0,2,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,5,1},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,5,1,1,1,1,1,2,0,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,5,1},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,5,1,1,1,1,1,2,0,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{48,32,3,3},TensorShape{48}},TensorShape{1,48,144,192},1,true,1,32,288,384,48,144,192,3,3,2,2,1,1,1,1,true,false,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{48,32,3,3},TensorShape{48}},TensorShape{1,48,144,192},1,true,1,32,288,384,48,144,192,3,3,2,2,1,1,1,1,true,false,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,1,5},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,1,5,1,1,1,1,0,2,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,5,1},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,5,1,1,1,1,1,2,0,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,5,1},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,5,1,1,1,1,1,2,0,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,1,5},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,1,5,1,1,1,1,0,2,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,1,5},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,1,5,1,1,1,1,0,2,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,5,1},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,5,1,1,1,1,1,2,0,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,1,5},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,1,5,1,1,1,1,0,2,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{32,16,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,16,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,1,5},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,1,5,1,1,1,1,0,2,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,8,288,384},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,288,384},1,true,1,8,288,384,8,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,48,144,192},TensorShape{32,48,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,48,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*1327104+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,16,144,192},TensorShape{32,48,3,3},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,48,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<48){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,16,144,192},TensorShape{32,48,3,3},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,48,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<48){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,1,5},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,1,5,1,1,1,1,0,2,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,288,384,32,144,192,3,3,2,2,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,4,288,384},TensorShape{32,4,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,4,288,384,32,288,384,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,1,5},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,1,5,1,1,1,1,0,2,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,5,1},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,5,1,1,1,1,1,2,0,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,8,288,384},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,288,384},1,true,1,8,288,384,8,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,8,288,384},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,288,384},1,true,1,8,288,384,8,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,5,1},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,5,1,1,1,1,1,2,0,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,5,1},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,5,1,1,1,1,1,2,0,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,8,288,384},TensorShape{8,8,3,3},TensorShape{8}},TensorShape{1,8,288,384},1,true,1,8,288,384,8,288,384,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{20,32,3,3}},TensorShape{1,20,144,192},1,false,1,32,144,192,20,144,192,3,3,1,1,1,1,1,1,true,false,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,1,5},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,1,5,1,1,1,1,0,2,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,144,192},1,true,1,16,144,192,16,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,144,192},1,true,1,16,144,192,16,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,5,1},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,5,1,1,1,1,1,2,0,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,5,1},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,5,1,1,1,1,1,2,0,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,1,5},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,1,5,1,1,1,1,0,2,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{1,32,144,192},TensorShape{32,64,5,1},TensorShape{32}},TensorShape{1,32,144,192},2,true,1,64,144,192,32,144,192,5,1,1,1,1,1,2,0,true,true,true,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<32){return readT1(i0,(i1-0),i2,i3);}if(i1<64){return readT2(i0,(i1-32),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,288,384},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,288,384,32,144,192,3,3,2,2,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*3538944+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,1,5},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,1,5,1,1,1,1,0,2,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,4,288,384},TensorShape{32,4,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,4,288,384,32,288,384,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,1,5},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,1,5,1,1,1,1,0,2,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,5,1},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,5,1,1,1,1,1,2,0,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3}},TensorShape{1,32,144,192},1,false,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,1,5},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,1,5,1,1,1,1,0,2,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,1,5},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,1,5,1,1,1,1,0,2,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,5,1},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,5,1,1,1,1,1,2,0,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,5,1},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,5,1,1,1,1,1,2,0,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,1,5},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,1,5,1,1,1,1,0,2,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,1,5},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,1,5,1,1,1,1,0,2,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,5,1},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,5,1,1,1,1,1,2,0,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,5,1},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,5,1,1,1,1,1,2,0,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{1,32,144,192},TensorShape{20,52,5,1},TensorShape{20}},TensorShape{1,20,144,192},2,true,1,52,144,192,20,144,192,5,1,1,1,1,1,2,0,true,false,false,R"PWLF(fn readT1(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
fn readT2(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL1(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{if(i1<20){return readT1(i0,(i1-0),i2,i3);}if(i1<52){return readT2(i0,(i1-20),i2,i3);}return 0.0; // unreachable under strict output shape
}
)PWLF","readT0"},
{{TensorShape{1,4,288,384},TensorShape{32,4,3,3},TensorShape{32}},TensorShape{1,32,288,384},1,true,1,4,288,384,32,288,384,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*110592+(i2)*384+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,32,144,192},TensorShape{32,32,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,32,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,true,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*884736+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{32,20,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,20,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{32,16,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,16,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{32,16,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,16,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,144,192},1,true,1,16,144,192,16,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,144,192},1,true,1,16,144,192,16,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,144,192},1,true,1,16,144,192,16,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{32,20,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,20,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,144,192},1,true,1,16,144,192,16,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,144,192},1,true,1,16,144,192,16,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,20,144,192},TensorShape{32,20,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,20,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*552960+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{32,16,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,16,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,144,192},1,true,1,16,144,192,16,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,144,192},1,true,1,16,144,192,16,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{32,16,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,16,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,144,192},1,true,1,16,144,192,16,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{32,16,3,3},TensorShape{32}},TensorShape{1,32,144,192},1,true,1,16,144,192,32,144,192,3,3,1,1,1,1,1,1,true,true,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,144,192},1,true,1,16,144,192,16,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,16,144,192},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,144,192},1,true,1,16,144,192,16,144,192,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(i0:i32,i1:i32,i2:i32,i3:i32)->f32{return readL0(((i0)*442368+(i1)*27648+(i2)*192+(i3)*1));}
)PWLF","readT0"},
{{TensorShape{1,64,288,384},TensorShape{1,64,576,768},TensorShape{16,64,3,3}},TensorShape{1,16,576,768},2,false,1,64,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,true,R"PWLF(fn readT0(n:i32,c:i32,y:i32,x:i32)->f32{let a=readL0(n*7077888+c*110592+(y/2)*384+x/2);let b=readL1(n*28311552+c*442368+y*768+x);return a+b;}
)PWLF","readT0"},
{{TensorShape{1,64,288,384},TensorShape{1,64,576,768}},TensorShape{1,64,576,768},2,false,1,64,576,768,64,576,768,1,1,1,1,1,1,0,0,true,true,true,R"PWLF(fn readT0(n:i32,c:i32,y:i32,x:i32)->f32{let a=readL0(n*7077888+c*110592+(y/2)*384+x/2);let b=readL1(n*28311552+c*442368+y*768+x);return a+b;}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,16,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(n:i32,c:i32,y:i32,x:i32)->f32{return readL0(n*7077888+c*442368+y*768+x);}
)PWLF","readT0"},
{{TensorShape{1,16,576,768},TensorShape{16,16,3,3},TensorShape{16}},TensorShape{1,16,576,768},1,true,1,16,576,768,16,576,768,3,3,1,1,1,1,1,1,true,false,false,R"PWLF(fn readT0(n:i32,c:i32,y:i32,x:i32)->f32{return readL0(n*7077888+c*442368+y*768+x);}
)PWLF","readT0"}
};return specs;}
