import Foundation
import CoreBluetooth
import Combine

class BluetoothManager: NSObject, ObservableObject {
    @Published var isConnected = false
    @Published var isScanning = false
    @Published var discoveredDevices: [CBPeripheral] = []
    @Published var isBluetoothReady = false
    
    // Synced values from ESP32
    @Published var currentBrightness: Double = 75
    @Published var currentRunning: Double = 30
    
    private var centralManager: CBCentralManager!
    private var gt3Peripheral: CBPeripheral?
    
    private var charMode: CBCharacteristic?
    private var charBrightness: CBCharacteristic?
    private var charRunning: CBCharacteristic?
    private var charColor: CBCharacteristic?
    private var charSpeed: CBCharacteristic?
    private var charOverride: CBCharacteristic?
    private var charAnimation: CBCharacteristic?
    private var charSaveDefault: CBCharacteristic?
    private var charManualBrake: CBCharacteristic?
    private var charManualLeft: CBCharacteristic?
    private var charManualRight: CBCharacteristic?
    private var charManualReverse: CBCharacteristic?
    private var charManualHazards: CBCharacteristic?
    private var charEffectBrightness: CBCharacteristic?
    private var charPoliceSegments: CBCharacteristic?
    private var charChaseWidth: CBCharacteristic?
    
    private let serviceUUID = CBUUID(string: "4fafc201-1fb5-459e-8fcc-c5c9c331914b")
    private let charModeUUID = CBUUID(string: "beb5483e-36e1-4688-b7f5-ea07361b26a8")
    private let charBrightnessUUID = CBUUID(string: "beb5483f-36e1-4688-b7f5-ea07361b26a8")
    private let charRunningUUID = CBUUID(string: "beb54840-36e1-4688-b7f5-ea07361b26a8")
    private let charColorUUID = CBUUID(string: "beb54841-36e1-4688-b7f5-ea07361b26a8")
    private let charSpeedUUID = CBUUID(string: "beb54842-36e1-4688-b7f5-ea07361b26a8")
    private let charOverrideUUID = CBUUID(string: "beb54843-36e1-4688-b7f5-ea07361b26a8")
    private let charAnimationUUID = CBUUID(string: "beb54844-36e1-4688-b7f5-ea07361b26a8")
    private let charSaveDefaultUUID = CBUUID(string: "beb54845-36e1-4688-b7f5-ea07361b26a8")
    private let charManualBrakeUUID = CBUUID(string: "beb54846-36e1-4688-b7f5-ea07361b26a8")
    private let charManualLeftUUID = CBUUID(string: "beb54847-36e1-4688-b7f5-ea07361b26a8")
    private let charManualRightUUID = CBUUID(string: "beb54848-36e1-4688-b7f5-ea07361b26a8")
    private let charManualReverseUUID = CBUUID(string: "beb54849-36e1-4688-b7f5-ea07361b26a8")
    private let charManualHazardsUUID = CBUUID(string: "beb5484a-36e1-4688-b7f5-ea07361b26a8")
    private let charEffectBrightnessUUID = CBUUID(string: "beb5484b-36e1-4688-b7f5-ea07361b26a8")
    private let charPoliceSegmentsUUID = CBUUID(string: "beb5484c-36e1-4688-b7f5-ea07361b26a8")
    private let charChaseWidthUUID = CBUUID(string: "beb5484d-36e1-4688-b7f5-ea07361b26a8")
    
    override init() {
        super.init()
        print("🔵 BluetoothManager: Starting initialization...")
        
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
            print("🔵 BluetoothManager: Creating CBCentralManager...")
            self.centralManager = CBCentralManager(delegate: self, queue: DispatchQueue.main)
            print("🔵 BluetoothManager: CBCentralManager created")
            
            DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) {
                self.checkBluetoothState()
            }
        }
    }

    func checkBluetoothState() {
        print("🔵 Manual state check...")
        if let manager = centralManager {
            if manager.state == .poweredOn && !isBluetoothReady {
                print("⚠️ Manually updating state...")
                DispatchQueue.main.async {
                    self.isBluetoothReady = true
                }
            }
        }
    }
    
    func startScan() {
        print("🔵 startScan called")
        guard isBluetoothReady else {
            print("❌ Bluetooth not ready")
            return
        }
        
        print("✅ Starting BLE scan...")
        DispatchQueue.main.async {
            self.discoveredDevices.removeAll()
            self.isScanning = true
        }
        centralManager.scanForPeripherals(withServices: nil, options: nil)
        
        DispatchQueue.main.asyncAfter(deadline: .now() + 30) {
            self.stopScan()
        }
    }
    
    func stopScan() {
        print("🔵 Stopping scan...")
        centralManager.stopScan()
        DispatchQueue.main.async {
            self.isScanning = false
        }
    }
    
    func connect(to peripheral: CBPeripheral) {
        print("🔵 Connecting to: \(peripheral.identifier)")
        gt3Peripheral = peripheral
        gt3Peripheral?.delegate = self
        centralManager.connect(peripheral, options: nil)
        stopScan()
    }
    
    func disconnect() {
        print("🔵 Disconnecting...")
        if let peripheral = gt3Peripheral {
            centralManager.cancelPeripheralConnection(peripheral)
        }
    }
    
    // Read current values from ESP32
    func syncValues() {
        print("📥 Syncing values from ESP32...")
        if let char = charBrightness {
            gt3Peripheral?.readValue(for: char)
        }
        if let char = charRunning {
            gt3Peripheral?.readValue(for: char)
        }
    }
    
    func setMode(_ mode: UInt8) {
        guard let char = charMode else { return }
        let data = Data([mode])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Sent mode: \(mode)")
    }
    
    func setBrightness(_ value: UInt8) {
        guard let char = charBrightness else { return }
        let data = Data([value])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Sent brightness: \(value)")
    }
    
    func setRunning(_ value: UInt8) {
        guard let char = charRunning else { return }
        let data = Data([value])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Sent running: \(value)")
    }
    
    func setColor(r: UInt8, g: UInt8, b: UInt8) {
        guard let char = charColor else { return }
        let data = Data([r, g, b])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Sent color: R:\(r) G:\(g) B:\(b)")
    }
    
    func setSpeed(_ value: UInt16) {
        guard let char = charSpeed else { return }
        let low = UInt8(value & 0xFF)
        let high = UInt8((value >> 8) & 0xFF)
        let data = Data([low, high])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Sent speed: \(value)")
    }
    
    func setOverride(_ enabled: Bool) {
        guard let char = charOverride else { return }
        let data = Data([enabled ? 1 : 0])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Sent override: \(enabled)")
    }
    
    func triggerAnimation() {
        print("📤 triggerAnimation called")
        guard let char = charAnimation else {
            print("❌ charAnimation is nil!")
            return
        }
        let data = Data([1])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Triggered animation")
    }
    
    func saveAsDefault() {
        print("📤 saveAsDefault called")
        guard let char = charSaveDefault else {
            print("❌ charSaveDefault is nil!")
            return
        }
        let data = Data([1])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Triggered save defaults")
    }
    
    // Manual signal controls
    func setManualBrake(_ enabled: Bool) {
        guard let char = charManualBrake else { return }
        let data = Data([enabled ? 1 : 0])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Manual brake: \(enabled)")
    }
    
    func setManualLeft(_ enabled: Bool) {
        guard let char = charManualLeft else { return }
        let data = Data([enabled ? 1 : 0])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Manual left: \(enabled)")
    }
    
    func setManualRight(_ enabled: Bool) {
        guard let char = charManualRight else { return }
        let data = Data([enabled ? 1 : 0])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Manual right: \(enabled)")
    }
    
    func setManualReverse(_ enabled: Bool) {
        guard let char = charManualReverse else { return }
        let data = Data([enabled ? 1 : 0])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Manual reverse: \(enabled)")
    }
    
    func setManualHazards(_ enabled: Bool) {
        guard let char = charManualHazards else { return }
        let data = Data([enabled ? 1 : 0])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Manual hazards: \(enabled)")
    }
    
    func setEffectBrightness(_ value: UInt8) {
        guard let char = charEffectBrightness else { return }
        let data = Data([value])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Effect brightness: \(value)%")
    }
    
    func setPoliceSegments(_ value: UInt8) {
        guard let char = charPoliceSegments else { return }
        let data = Data([value])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Police segments: \(value)")
    }
    
    func setChaseWidth(_ value: UInt8) {
        guard let char = charChaseWidth else { return }
        let data = Data([value])
        gt3Peripheral?.writeValue(data, for: char, type: .withResponse)
        print("📤 Chase width: \(value)")
    }
}

extension BluetoothManager: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        print("🟢 centralManagerDidUpdateState CALLED")
        
        DispatchQueue.main.async {
            self.isBluetoothReady = (central.state == .poweredOn)
            print("🟢 isBluetoothReady set to: \(self.isBluetoothReady)")
        }
        
        switch central.state {
        case .poweredOn:
            print("✅ Bluetooth is ON and READY!")
        case .poweredOff:
            print("❌ Bluetooth is OFF")
        case .unsupported:
            print("❌ Bluetooth not supported")
        case .unauthorized:
            print("❌ Bluetooth permission denied")
        case .resetting:
            print("⚠️ Bluetooth is resetting...")
        case .unknown:
            print("⚠️ Bluetooth state unknown")
        @unknown default:
            print("⚠️ Unknown Bluetooth state")
        }
    }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        let deviceName = peripheral.name ?? (advertisementData[CBAdvertisementDataLocalNameKey] as? String)
        
        if deviceName == "GT3_Wing" {
            print("✅ FOUND GT3_Wing!")
            DispatchQueue.main.async {
                if !self.discoveredDevices.contains(where: { $0.identifier == peripheral.identifier }) {
                    var devices = self.discoveredDevices
                    devices.append(peripheral)
                    self.discoveredDevices = devices
                    print("✅ Added to list. Total: \(self.discoveredDevices.count)")
                    self.objectWillChange.send()
                }
            }
        }
    }
    
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        print("✅ Connected to: \(peripheral.name ?? "device")")
        DispatchQueue.main.async {
            self.isConnected = true
        }
        peripheral.discoverServices([serviceUUID])
    }
    
    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        print("❌ Disconnected")
        if let error = error {
            print("❌ Error: \(error.localizedDescription)")
        }
        DispatchQueue.main.async {
            self.isConnected = false
            self.gt3Peripheral = nil
        }
    }
    
    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        print("❌ Failed to connect")
        if let error = error {
            print("❌ Error: \(error.localizedDescription)")
        }
    }
}

extension BluetoothManager: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        print("🔍 Discovering services...")
        if let error = error {
            print("❌ Service discovery error: \(error.localizedDescription)")
            return
        }
        
        guard let services = peripheral.services else {
            print("❌ No services found")
            return
        }
        
        print("✅ Found \(services.count) service(s)")
        for service in services {
            if service.uuid == serviceUUID {
                print("✅ GT3 Service found!")
                peripheral.discoverCharacteristics(nil, for: service)
            }
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        print("🔍 Discovering characteristics...")
        if let error = error {
            print("❌ Error: \(error.localizedDescription)")
            return
        }
        
        guard let characteristics = service.characteristics else {
            print("❌ No characteristics")
            return
        }
        
        print("✅ Found \(characteristics.count) characteristic(s)")
        for char in characteristics {
            switch char.uuid {
            case charModeUUID:
                charMode = char
                print("  ✅ Mode")
            case charBrightnessUUID:
                charBrightness = char
                // Enable notifications
                peripheral.setNotifyValue(true, for: char)
                print("  ✅ Brightness (notify enabled)")
            case charRunningUUID:
                charRunning = char
                // Enable notifications
                peripheral.setNotifyValue(true, for: char)
                print("  ✅ Running (notify enabled)")
            case charColorUUID:
                charColor = char
                print("  ✅ Color")
            case charSpeedUUID:
                charSpeed = char
                print("  ✅ Speed")
            case charOverrideUUID:
                charOverride = char
                print("  ✅ Override")
            case charAnimationUUID:
                charAnimation = char
                print("  ✅ Animation")
            case charSaveDefaultUUID:
                charSaveDefault = char
                print("  ✅ Save Default")
            case charManualBrakeUUID:
                charManualBrake = char
                print("  ✅ Manual Brake")
            case charManualLeftUUID:
                charManualLeft = char
                print("  ✅ Manual Left")
            case charManualRightUUID:
                charManualRight = char
                print("  ✅ Manual Right")
            case charManualReverseUUID:
                charManualReverse = char
                print("  ✅ Manual Reverse")
            case charManualHazardsUUID:
                charManualHazards = char
                print("  ✅ Manual Hazards")
            case charEffectBrightnessUUID:
                charEffectBrightness = char
                print("  ✅ Effect Brightness")
            case charPoliceSegmentsUUID:
                charPoliceSegments = char
                print("  ✅ Police Segments")
            case charChaseWidthUUID:
                charChaseWidth = char
                print("  ✅ Chase Width")
            default:
                break
            }
        }
        
        print("🎉 All characteristics ready!")
        // Sync values from ESP32
        syncValues()
    }
    
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        if let error = error {
            print("❌ Read error: \(error.localizedDescription)")
            return
        }
        
        guard let data = characteristic.value else {
            print("❌ No data received")
            return
        }
        
        // Handle brightness value
        if characteristic.uuid == charBrightnessUUID, let value = data.first {
            let percentage = (Double(value) / 255.0) * 100.0
            DispatchQueue.main.async {
                self.currentBrightness = percentage
                print("📥 Received brightness: \(value) (\(Int(percentage))%)")
            }
        }
        // Handle running brightness value
        else if characteristic.uuid == charRunningUUID, let value = data.first {
            DispatchQueue.main.async {
                self.currentRunning = Double(value)
                print("📥 Received running: \(value)%")
            }
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        if let error = error {
            print("❌ Write error: \(error.localizedDescription)")
        } else {
            print("✅ Write successful")
        }
    }
}

