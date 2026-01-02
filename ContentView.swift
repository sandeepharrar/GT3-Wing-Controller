import SwiftUI

struct ContentView: View {
    @StateObject private var bluetooth = BluetoothManager()
    
    var body: some View {
        NavigationView {
            Group {
                if bluetooth.isConnected {
                    MainControlView()
                        .environmentObject(bluetooth)
                } else {
                    ConnectionView()
                        .environmentObject(bluetooth)
                }
            }
            .navigationTitle("GT3 Wing")
            .navigationBarTitleDisplayMode(.inline)
        }
    }
}

// MARK: - Connection View
struct ConnectionView: View {
    @EnvironmentObject var bluetooth: BluetoothManager
    
    var body: some View {
        VStack(spacing: 30) {
            headerSection
            
            if bluetooth.isScanning {
                scanningSection
            } else {
                if bluetooth.isBluetoothReady {
                    scanButton
                } else {
                    Text("Waiting for Bluetooth...")
                        .foregroundColor(.gray)
                }
            }
            
            deviceList
            
            Spacer()
            
            // Developer Credit
            VStack(spacing: 4) {
                Text("Developed and Tested by Sandeep Harrar v1.0")
                    .font(.caption2)
                    .foregroundColor(.secondary)
                Text("and Claude AI")
                    .font(.caption2)
                    .foregroundColor(.secondary)
            }
            .padding(.bottom, 8)
        }
        .padding()
    }
    
    private var headerSection: some View {
        VStack(spacing: 10) {
            Image(systemName: "antenna.radiowaves.left.and.right")
                .font(.system(size: 80))
                .foregroundColor(.blue)
            
            Text("GT3 Wing Controller")
                .font(.title2)
                .fontWeight(.bold)
        }
    }
    
    private var scanningSection: some View {
        VStack(spacing: 15) {
            ProgressView()
                .scaleEffect(1.5)
            Text("Scanning for GT3_Wing...")
                .foregroundColor(.secondary)
                .font(.subheadline)
        }
    }
    
    private var scanButton: some View {
        Button(action: {
            bluetooth.startScan()
        }) {
            HStack {
                Image(systemName: "magnifyingglass")
                Text("Scan for Device")
            }
            .font(.headline)
            .foregroundColor(.white)
            .frame(maxWidth: .infinity)
            .padding()
            .background(Color.blue)
            .cornerRadius(12)
        }
        .padding(.horizontal, 40)
    }
    
    private var deviceList: some View {
        VStack(alignment: .leading, spacing: 10) {
            if bluetooth.discoveredDevices.count > 0 {
                Text("Found Devices")
                    .font(.headline)
                    .padding(.horizontal)
                
                ForEach(0..<bluetooth.discoveredDevices.count, id: \.self) { index in
                    deviceRow(at: index)
                }
            }
        }
    }
    
    private func deviceRow(at index: Int) -> some View {
        let device = bluetooth.discoveredDevices[index]
        
        return Button(action: {
            bluetooth.connect(to: device)
        }) {
            HStack {
                Image(systemName: "car.fill")
                    .foregroundColor(.blue)
                Text("GT3_Wing")
                    .fontWeight(.medium)
                Spacer()
                Image(systemName: "chevron.right")
                    .foregroundColor(.secondary)
            }
            .padding()
            .background(Color(.systemGray6))
            .cornerRadius(10)
        }
        .buttonStyle(PlainButtonStyle())
        .padding(.horizontal)
    }
}

// MARK: - Main Control View (Always Available)
struct MainControlView: View {
    @EnvironmentObject var bluetooth: BluetoothManager
    @State private var showingOverride = false
    
    var body: some View {
        ScrollView {
            VStack(spacing: 20) {
                connectionStatus
                
                brightnessControl
                runningControl
                
                saveDefaultButton
                
                overrideButton
                
                Spacer()
                    .frame(height: 20)
                
                // Developer Credit
                VStack(spacing: 4) {
                    Text("Developed and Tested by Sandeep Harrar v1.0")
                        .font(.caption2)
                        .foregroundColor(.secondary)
                    Text("and Claude AI")
                        .font(.caption2)
                        .foregroundColor(.secondary)
                }
                .padding(.bottom, 8)
            }
            .padding()
        }
        .sheet(isPresented: $showingOverride) {
            OverrideControlView()
                .environmentObject(bluetooth)
        }
    }
    
    private var connectionStatus: some View {
        HStack {
            HStack(spacing: 8) {
                Circle()
                    .fill(Color.green)
                    .frame(width: 10, height: 10)
                Text("Connected")
                    .font(.subheadline)
                    .foregroundColor(.secondary)
            }
            
            Spacer()
            
            Button(action: {
                bluetooth.disconnect()
            }) {
                Text("Disconnect")
                    .font(.subheadline)
                    .foregroundColor(.red)
            }
        }
        .padding()
        .background(Color(.systemGray6))
        .cornerRadius(10)
    }
    
    private var brightnessControl: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Text("Master Brightness")
                    .font(.headline)
                Spacer()
                Text("\(Int(bluetooth.currentBrightness))%")
                    .font(.headline)
                    .foregroundColor(.blue)
            }
            
            HStack(spacing: 12) {
                Image(systemName: "sun.min")
                    .foregroundColor(.secondary)
                    .font(.caption)
                
                Slider(value: $bluetooth.currentBrightness, in: 0...100, step: 5)
                    .tint(.blue)
                    .onChange(of: bluetooth.currentBrightness) { _, newValue in
                        let brightnessValue = UInt8((newValue / 100) * 255)
                        bluetooth.setBrightness(brightnessValue)
                    }
                
                Image(systemName: "sun.max")
                    .foregroundColor(.secondary)
                    .font(.caption)
            }
        }
        .padding()
        .background(Color(.systemGray6))
        .cornerRadius(10)
    }
    
    private var runningControl: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Text("Running Light Dim")
                    .font(.headline)
                Spacer()
                Text("\(Int(bluetooth.currentRunning))%")
                    .font(.headline)
                    .foregroundColor(.orange)
            }
            
            HStack(spacing: 12) {
                Image(systemName: "sun.min")
                    .foregroundColor(.secondary)
                    .font(.caption)
                
                Slider(value: $bluetooth.currentRunning, in: 0...100, step: 5)
                    .tint(.orange)
                    .onChange(of: bluetooth.currentRunning) { _, newValue in
                        bluetooth.setRunning(UInt8(newValue))
                    }
                
                Image(systemName: "sun.max")
                    .foregroundColor(.secondary)
                    .font(.caption)
            }
        }
        .padding()
        .background(Color(.systemGray6))
        .cornerRadius(10)
    }
    
    private var saveDefaultButton: some View {
        Button(action: {
            bluetooth.saveAsDefault()
        }) {
            HStack {
                Image(systemName: "checkmark.circle.fill")
                Text("Save as Default")
            }
            .font(.headline)
            .foregroundColor(.white)
            .frame(maxWidth: .infinity)
            .padding()
            .background(Color.blue)
            .cornerRadius(12)
        }
    }
    
    private var overrideButton: some View {
        Button(action: {
            showingOverride = true
        }) {
            HStack {
                Image(systemName: "slider.horizontal.3")
                Text("Advanced Controls")
                Spacer()
                Image(systemName: "lock.fill")
                    .font(.caption)
            }
            .font(.headline)
            .foregroundColor(.white)
            .frame(maxWidth: .infinity)
            .padding()
            .background(Color.blue)
            .cornerRadius(12)
        }
    }
}

// MARK: - Override Control View (Advanced Controls)
struct OverrideControlView: View {
    @EnvironmentObject var bluetooth: BluetoothManager
    @Environment(\.dismiss) var dismiss
    
    @State private var overrideEnabled: Bool = false
    @State private var selectedMode: Int = 0
    @State private var selectedColor: Color = .red
    @State private var animSpeed: Double = 50
    @State private var effectBrightness: Double = 100 // Percentage of global brightness
    @State private var policeSegments: Double = 6 // Number of segments for police mode
    @State private var chaseWidth: Double = 5 // Number of LEDs in chase effect
    
    // Manual signal states
    @State private var brakeActive: Bool = false
    @State private var leftActive: Bool = false
    @State private var rightActive: Bool = false
    @State private var reverseActive: Bool = false
    @State private var hazardsActive: Bool = false
    
    let modes = ["Normal", "Rainbow", "Chase", "Police", "Strobe", "Solid"]
    
    var body: some View {
        NavigationView {
            ScrollView {
                VStack(spacing: 20) {
                    overrideToggle
                    
                    if overrideEnabled {
                        animationButton
                        modeSelector
                        
                        // Color picker - hide for Normal (0), Rainbow (1), and Police (3)
                        if selectedMode != 0 && selectedMode != 1 && selectedMode != 3 {
                            colorPicker
                        }
                        
                        // Speed slider - hide for Normal (0)
                        if selectedMode != 0 {
                            speedSlider
                        }
                        
                        // Police segments - only show for Police mode (3)
                        if selectedMode == 3 {
                            policeSegmentsSlider
                        }
                        
                        // Chase width - only show for Chase mode (2)
                        if selectedMode == 2 {
                            chaseWidthSlider
                        }
                        
                        // Effect brightness only shows when not in Normal mode
                        if selectedMode != 0 {
                            effectBrightnessSlider
                        }
                        
                        Divider()
                            .padding(.vertical)
                        
                        Text("Manual Signal Controls")
                            .font(.headline)
                            .frame(maxWidth: .infinity, alignment: .leading)
                        
                        manualControls
                    }
                }
                .padding()
            }
            .navigationTitle("Advanced Controls")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Done") {
                        dismiss()
                    }
                }
            }
            .onDisappear {
                // When the sheet closes, disable override and reset to Normal mode
                if overrideEnabled {
                    overrideEnabled = false
                    bluetooth.setOverride(false)
                    
                    // Reset manual controls
                    brakeActive = false
                    leftActive = false
                    rightActive = false
                    reverseActive = false
                    hazardsActive = false
                    bluetooth.setManualBrake(false)
                    bluetooth.setManualLeft(false)
                    bluetooth.setManualRight(false)
                    bluetooth.setManualReverse(false)
                    bluetooth.setManualHazards(false)
                }
                
                // Always reset to Normal mode when closing
                selectedMode = 0
                bluetooth.setMode(0)
            }
        }
    }
    
    private var overrideToggle: some View {
        VStack(alignment: .leading, spacing: 8) {
            Toggle(isOn: $overrideEnabled) {
                VStack(alignment: .leading, spacing: 4) {
                    HStack {
                        Image(systemName: overrideEnabled ? "lock.open.fill" : "lock.fill")
                            .foregroundColor(overrideEnabled ? .green : .red)
                        Text("Override Mode")
                            .font(.headline)
                    }
                    Text(overrideEnabled ? "Manual control active - car signals disabled" : "Tap to enable advanced controls")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }
            .tint(.blue)
            .onChange(of: overrideEnabled) { _, newValue in
                bluetooth.setOverride(newValue)
                if !newValue {
                    // Reset all manual controls when disabling override
                    brakeActive = false
                    leftActive = false
                    rightActive = false
                    reverseActive = false
                    hazardsActive = false
                    bluetooth.setManualBrake(false)
                    bluetooth.setManualLeft(false)
                    bluetooth.setManualRight(false)
                    bluetooth.setManualReverse(false)
                    bluetooth.setManualHazards(false)
                    
                    // Reset mode back to Normal
                    selectedMode = 0
                    bluetooth.setMode(0)
                }
            }
        }
        .padding()
        .background(Color(.systemGray6))
        .cornerRadius(10)
    }
    
    private var animationButton: some View {
        Button(action: {
            bluetooth.triggerAnimation()
        }) {
            HStack {
                Image(systemName: "play.circle.fill")
                Text("Play Boot Animation")
            }
            .font(.headline)
            .foregroundColor(.white)
            .frame(maxWidth: .infinity)
            .padding()
            .background(Color.blue)
            .cornerRadius(12)
        }
    }
    
    private var modeSelector: some View {
        VStack(alignment: .leading, spacing: 10) {
            Text("Light Mode")
                .font(.headline)
            
            Picker("Mode", selection: $selectedMode) {
                ForEach(0..<modes.count, id: \.self) { index in
                    Text(modes[index]).tag(index)
                }
            }
            .pickerStyle(SegmentedPickerStyle())
            .onChange(of: selectedMode) { _, newValue in
                bluetooth.setMode(UInt8(newValue))
            }
        }
        .padding()
        .background(Color(.systemGray6))
        .cornerRadius(10)
    }
    
    private var colorPicker: some View {
        VStack(alignment: .leading, spacing: 10) {
            Text("Custom Color")
                .font(.headline)
            
            ColorPicker("Choose Color", selection: $selectedColor)
                .onChange(of: selectedColor) { _, newColor in
                    let components = UIColor(newColor).cgColor.components ?? [0,0,0,1]
                    let r = UInt8(components[0] * 255)
                    let g = UInt8(components[1] * 255)
                    let b = UInt8(components[2] * 255)
                    bluetooth.setColor(r: r, g: g, b: b)
                }
        }
        .padding()
        .background(Color(.systemGray6))
        .cornerRadius(10)
    }
    
    private var speedSlider: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Text("Animation Speed")
                    .font(.headline)
                Spacer()
                Text("\(Int(animSpeed))%")
                    .font(.headline)
                    .foregroundColor(.blue)
            }
            
            HStack(spacing: 12) {
                Image(systemName: "tortoise")
                    .foregroundColor(.secondary)
                    .font(.caption)
                
                Slider(value: $animSpeed, in: 0...100, step: 5)
                    .tint(.blue)
                    .onChange(of: animSpeed) { _, newValue in
                        let delayMs = UInt16(5000 - (newValue * 45))
                        bluetooth.setSpeed(delayMs)
                    }
                
                Image(systemName: "hare")
                    .foregroundColor(.secondary)
                    .font(.caption)
            }
        }
        .padding()
        .background(Color(.systemGray6))
        .cornerRadius(10)
    }
    
    private var policeSegmentsSlider: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Text("Number of Segments")
                    .font(.headline)
                Spacer()
                Text("\(Int(policeSegments))")
                    .font(.headline)
                    .foregroundColor(.red)
            }
            
            HStack(spacing: 12) {
                Text("2")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Slider(value: $policeSegments, in: 2...12, step: 2)
                    .tint(.red)
                    .onChange(of: policeSegments) { _, newValue in
                        bluetooth.setPoliceSegments(UInt8(newValue))
                    }
                
                Text("12")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
        .padding()
        .background(Color(.systemGray6))
        .cornerRadius(10)
    }
    
    private var chaseWidthSlider: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Text("Chase Width")
                    .font(.headline)
                Spacer()
                Text("\(Int(chaseWidth)) LEDs")
                    .font(.headline)
                    .foregroundColor(.green)
            }
            
            HStack(spacing: 12) {
                Text("1")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Slider(value: $chaseWidth, in: 1...20, step: 1)
                    .tint(.green)
                    .onChange(of: chaseWidth) { _, newValue in
                        bluetooth.setChaseWidth(UInt8(newValue))
                    }
                
                Text("20")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
        .padding()
        .background(Color(.systemGray6))
        .cornerRadius(10)
    }
    
    private var effectBrightnessSlider: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Text("Effect Brightness")
                    .font(.headline)
                Spacer()
                Text("\(Int(effectBrightness))%")
                    .font(.headline)
                    .foregroundColor(.purple)
            }
            
            Text("Percentage of master brightness")
                .font(.caption)
                .foregroundColor(.secondary)
            
            HStack(spacing: 12) {
                Image(systemName: "sun.min")
                    .foregroundColor(.secondary)
                    .font(.caption)
                
                Slider(value: $effectBrightness, in: 0...100, step: 5)
                    .tint(.purple)
                    .onChange(of: effectBrightness) { _, newValue in
                        bluetooth.setEffectBrightness(UInt8(newValue))
                    }
                
                Image(systemName: "sun.max")
                    .foregroundColor(.secondary)
                    .font(.caption)
            }
        }
        .padding()
        .background(Color(.systemGray6))
        .cornerRadius(10)
    }
    
    private var manualControls: some View {
        VStack(spacing: 12) {
            // Brake
            signalButton(
                title: "Brake Light",
                icon: "exclamationmark.triangle.fill",
                isActive: $brakeActive,
                color: .red
            ) {
                brakeActive.toggle()
                bluetooth.setManualBrake(brakeActive)
            }
            
            HStack(spacing: 12) {
                // Left Indicator
                signalButton(
                    title: "Left",
                    icon: "arrow.left",
                    isActive: $leftActive,
                    color: .orange
                ) {
                    leftActive.toggle()
                    if leftActive && hazardsActive {
                        hazardsActive = false
                        bluetooth.setManualHazards(false)
                    }
                    bluetooth.setManualLeft(leftActive)
                }
                
                // Right Indicator
                signalButton(
                    title: "Right",
                    icon: "arrow.right",
                    isActive: $rightActive,
                    color: .orange
                ) {
                    rightActive.toggle()
                    if rightActive && hazardsActive {
                        hazardsActive = false
                        bluetooth.setManualHazards(false)
                    }
                    bluetooth.setManualRight(rightActive)
                }
            }
            
            // Hazards
            signalButton(
                title: "Hazard Lights",
                icon: "exclamationmark.triangle.fill",
                isActive: $hazardsActive,
                color: .orange
            ) {
                hazardsActive.toggle()
                if hazardsActive {
                    leftActive = false
                    rightActive = false
                    bluetooth.setManualLeft(false)
                    bluetooth.setManualRight(false)
                }
                bluetooth.setManualHazards(hazardsActive)
            }
            
            // Reverse
            signalButton(
                title: "Reverse Light",
                icon: "arrow.uturn.backward",
                isActive: $reverseActive,
                color: .white
            ) {
                reverseActive.toggle()
                bluetooth.setManualReverse(reverseActive)
            }
        }
    }
    
    private func signalButton(
        title: String,
        icon: String,
        isActive: Binding<Bool>,
        color: Color,
        action: @escaping () -> Void
    ) -> some View {
        Button(action: action) {
            HStack {
                Image(systemName: icon)
                    .font(.title3)
                Text(title)
                    .font(.headline)
                Spacer()
                if isActive.wrappedValue {
                    Image(systemName: "checkmark.circle.fill")
                        .foregroundColor(.green)
                }
            }
            .foregroundColor(isActive.wrappedValue ? .white : color)
            .padding()
            .frame(maxWidth: .infinity)
            .background(
                isActive.wrappedValue
                    ? AnyShapeStyle(color)
                    : AnyShapeStyle(Color(.systemGray6))
            )
            .cornerRadius(10)
            .overlay(
                RoundedRectangle(cornerRadius: 10)
                    .stroke(color, lineWidth: isActive.wrappedValue ? 0 : 2)
            )
        }
    }
}

