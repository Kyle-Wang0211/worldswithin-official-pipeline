import Flutter
import UIKit

@main
@objc class AppDelegate: FlutterAppDelegate {
  override func application(
    _ application: UIApplication,
    didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
  ) -> Bool {
    GeneratedPluginRegistrant.register(with: self)

                                      
                                                                    
                                                                      
                                                                            
                                                                              
                                      
                                                                    
                                                      
    if let registrar = self.registrar(forPlugin: "AetherTexturePlugin") {
      AetherTexturePlugin.register(with: registrar)
    } else {
      NSLog("[AppDelegate] registrar(forPlugin: AetherTexturePlugin) nil — texture widget will be blank")
    }

                                                                                                      
                                                                           
                                                                                                   
    if let registrar = self.registrar(forPlugin: "PwVioTimebasePlugin") {
      PwVioTimebasePlugin.register(with: registrar)
    } else {
      NSLog("[AppDelegate] registrar(forPlugin: PwVioTimebasePlugin) nil — VIO 时基无法测量,采集侧将无法判定域错配")
    }

                                                                                      
                                                      
    if let registrar = self.registrar(forPlugin: "PwVioThermalPlugin") {
      PwVioThermalPlugin.register(with: registrar)
    } else {
      NSLog("[AppDelegate] registrar(forPlugin: PwVioThermalPlugin) nil — VIO thermal telemetry unavailable")
    }
                                                                                      
                                                                                           
                                                                                        
    if let registrar = self.registrar(forPlugin: "PwLodTexturePlugin") {
      PwLodTexturePlugin.register(with: registrar)
    } else {
      NSLog("[AppDelegate] registrar(forPlugin: PwLodTexturePlugin) nil — cloud views fall back to the CPU painter")
    }
    if #available(iOS 11.0, *) {
      if let registrar = self.registrar(
        forPlugin: "OfficialAetherARKitPlugin"
      ) {
        OfficialAetherARKitPlugin.register(with: registrar)
      } else {
        NSLog("[AppDelegate] registrar(forPlugin: OfficialAetherARKitPlugin) nil — official capture route unavailable")
      }
    }

                                                                           
                                                                               
                                                                        
    if #available(iOS 13.0, *) {
      if let registrar = self.registrar(
        forPlugin: "OfficialArchiveBackgroundTask"
      ) {
        OfficialArchiveBackgroundTask.shared.register(with: registrar)
      } else {
        NSLog("[AppDelegate] archive background registrar unavailable")
      }
    }

                                                                                
                                                                           
                                                                         
    if #available(iOS 26.0, *) {
      OfficialReconUmbrella.shared.register()
    }

    return super.application(application, didFinishLaunchingWithOptions: launchOptions)
  }
}
