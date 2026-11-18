                                                                       
   
                                                                            
                                                                      
                                                                              
                                                                                 
                                                                         
                                                                               
                                                         
const int kOfficialMinimumCaptureFrames = 1;
const double kOfficialGlobalBaGrowthRatio = 1.40;

bool officialCaptureCanFinish({required int acceptedFrameCount}) =>
    acceptedFrameCount >= kOfficialMinimumCaptureFrames;

                                                          
bool officialCaptureCanShoot({required int acceptedFrameCount}) =>
    acceptedFrameCount >= 0;

class OfficialLiveSfmPublishPolicy {
  int _publishedRegisteredFrames = 0;
  int _publishedPointCount = 0;
  int _version = 0;

  int get version => _version;
  int get publishedRegisteredFrames => _publishedRegisteredFrames;
  int get publishedPointCount => _publishedPointCount;

  bool shouldRunGlobalBa({
    required int registeredFrames,
    required int pointCount,
  }) {
    if (registeredFrames < kOfficialMinimumCaptureFrames || pointCount <= 0) {
      return false;
    }
    if (_version == 0) return true;

    return registeredFrames >=
            _publishedRegisteredFrames * kOfficialGlobalBaGrowthRatio ||
        pointCount >= _publishedPointCount * kOfficialGlobalBaGrowthRatio;
  }

                                                                              
                                                                               
                                                                       
  void markGlobalBaPublished({
    required int registeredFrames,
    required int pointCount,
  }) {
    if (registeredFrames < kOfficialMinimumCaptureFrames || pointCount <= 0) {
      throw ArgumentError(
        'A stable official cloud requires registered frames and points',
      );
    }
    _publishedRegisteredFrames = registeredFrames;
    _publishedPointCount = pointCount;
    _version++;
  }
}
