//
//  MandelView.swift
//  MandelbrotSet
//
//  Created by Matt Welsh on 1/2/20.
//  Copyright © 2020 Team Sidney Enterprises. All rights reserved.
//

import Cocoa
import simd

class MandelView: NSView {
    
    private var curPalette: String = "Rainbow"
    private var maxIterations: UInt32 = 100
    private var curX: Double = -2.0
    private var curY: Double = -1.5
    private var curWidth: Double = 3.0
    private var curHeight: Double = 3.0
    // Colors are stored as RGBA values.
    private var curColors = Array(repeating: Array(repeating: UInt8(0), count: 4), count: 100)
    //private var curColors: [NSColor] = (0..<100).map { index in return NSColor(red: 0, green: 0, blue: 0, alpha: 1) }
    // Black default color in RGBA format.
    private let blackColor = [UInt8(0), UInt8(0), UInt8(0), UInt8(255)]
    private var mandelRendered: Bool = false
    private var renderInProgress: Bool = false
    private var mandelContext: CGContext?
    
    @IBOutlet weak var homeButton: NSButton!
    @IBOutlet weak var palettePopup: NSPopUpButton!
    @IBOutlet weak var progressIndicator: NSProgressIndicator!
    @IBOutlet weak var iterationsSlider: NSSlider!
    @IBOutlet weak var iterationsTextField: NSTextField!
    
    func makeMandelContext() {
        let scale: CGFloat = 1
        let colorSpace = CGColorSpaceCreateDeviceRGB()
        let bitmapInfo = CGImageAlphaInfo.noneSkipLast.rawValue
        self.mandelContext = CGContext(
            data: nil,
            width: Int(frame.width * scale),
            height: Int(frame.height * scale),
            bitsPerComponent: 8,
            bytesPerRow: 0,
            space: colorSpace,
            bitmapInfo: bitmapInfo
            )!
    }
    
    override init(frame: NSRect) {
        super.init(frame: frame)
        makeMandelContext()
    }
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        makeMandelContext()
    }

    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)
        //print("MandelView: Draw called with \(dirtyRect.minX) \(dirtyRect.minY) \(dirtyRect.maxX) \(dirtyRect.maxY)")
        
        if mandelRendered {
            let t0 = Date()
            guard let img = self.mandelContext?.makeImage() else {
                return
            }
            NSGraphicsContext.current?.cgContext.draw(img, in: dirtyRect)
            let t1 = Date()
            let ms = Int(t1.timeIntervalSince(t0) * 1000.0)
            //print("doRender: drawing done in \(ms) milliseconds")
        } else {
            doRender()
        }
        
        if let rect = selectionRect {
            let path = NSBezierPath(rect: rect)
            NSColor.selectedContentBackgroundColor.withAlphaComponent(0.2).setFill()
            NSColor.selectedContentBackgroundColor.setStroke()
            path.lineWidth = 1
            path.fill()
            path.stroke()
        }
        
        if let scrollFrom = scrollStart, let scrollTo = scrollEnd {
            NSColor.selectedContentBackgroundColor.withAlphaComponent(0.2).setFill()
            NSColor.selectedContentBackgroundColor.setStroke()
            let startPoint = NSRect(x: scrollFrom.x - 5, y: scrollFrom.y - 5, width: 10, height: 10)
            let path = NSBezierPath(ovalIn: startPoint)
            path.move(to: NSPoint(x: scrollFrom.x, y: scrollFrom.y))
            path.line(to: scrollTo)
            path.appendOval(in: NSRect(x: scrollTo.x - 5, y: scrollTo.y - 5, width: 10, height: 10))
            path.fill()
            path.stroke()
        }
    }
    
    func disableControls() {
        self.iterationsSlider.isEnabled = false
        self.iterationsTextField.isEnabled = false
        self.palettePopup.isEnabled = false
        self.homeButton.isEnabled = false
    }
    
    func enableControls() {
        self.iterationsSlider.isEnabled = true
        self.iterationsTextField.isEnabled = true
        self.palettePopup.isEnabled = true
        self.homeButton.isEnabled = true
    }
    
    @IBAction func handlePaletteSelection(_ sender: Any) {
        if let palette = palettePopup.titleOfSelectedItem {
            self.setPalette(palette)
        }
    }
    
    func setPalette(_ paletteName: String) {
        print("MandelView: setting palette to \(paletteName)")
        curPalette = paletteName
        makePalette(curPalette)
        doRender()
    }
    
    func makePalette(_ paletteName: String) {
        // See: https://stackoverflow.com/questions/448125/how-to-get-pixel-data-from-a-uiimage-cocoa-touch-or-cgimage-core-graphics
        let scale: CGFloat = 1
        let bounds = CGRect(x: 0, y: 0, width: 100, height: 1)
        let colorSpace = CGColorSpaceCreateDeviceRGB()
        let bitmapInfo = CGImageAlphaInfo.noneSkipLast.rawValue
        let context = CGContext(
            data: nil,
            width: Int(bounds.width * scale),
            height: Int(bounds.height * scale),
            bitsPerComponent: 8,
            bytesPerRow: 0,
            space: colorSpace,
            bitmapInfo: bitmapInfo
            )!
        
        var colors: [CGColor]
        var colorLocations: [CGFloat] = [0.0, 1.0]
        switch curPalette {
        case "Red":
            colors = [NSColor.black.cgColor, NSColor.red.cgColor]
        case "Blue":
             colors = [NSColor.black.cgColor, NSColor.blue.cgColor]
        case "Rainbow":
            colors = [NSColor.red.cgColor, NSColor.orange.cgColor, NSColor.yellow.cgColor, NSColor.green.cgColor, NSColor.blue.cgColor, NSColor.purple.cgColor]
            colorLocations = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]
        default:
            colors = [NSColor.black.cgColor, NSColor.white.cgColor]
        }
        let gradient = CGGradient(colorsSpace: colorSpace,
                                  colors: colors as CFArray,
                                  locations: colorLocations)!
        let startPoint = CGPoint.zero
        let endPoint = CGPoint(x: bounds.width, y: 0)
        context.drawLinearGradient(gradient,
                                   start: startPoint,
                                   end: endPoint,
                                   options: [])
        if let data = context.data {
            let ptr = data.assumingMemoryBound(to: UInt8.self)
            for colorIndex in 0..<100 {
                let i = colorIndex * 4
                curColors[colorIndex][0] = ptr[i]   // red
                curColors[colorIndex][1] = ptr[i+1] // green
                curColors[colorIndex][2] = ptr[i+2] // blue
                curColors[colorIndex][3] = ptr[i+3] // alpha
            }
        }
    }
    
    func doRender() {
        if renderInProgress {
            return
        }
        renderInProgress = true
        mandelRendered = false
        disableControls()
        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            guard let self = self else {
                return
            }
            print("doRender: Calling render")
            let t0 = Date()
            //self.renderDefault()
            self.renderSimd()
            let t1 = Date()
            let ms = Int(t1.timeIntervalSince(t0) * 1000.0)
            print("doRender: render done in \(ms) milliseconds")
            self.renderInProgress = false
            self.mandelRendered = true
            DispatchQueue.main.async {
                self.enableControls()
                self.needsDisplay = true
            }
        }
    }
    
    func renderDefault() {
        if let ctx = mandelContext {
            for y in 0..<ctx.height {
                // Note that the byte ordering of the bitmap context in memory starts with pixel (0, 0) in the
                // upper left corner, whereas the MacOS pixel coordinate system has (0, 0) at the lower left corner.
                // So, we flip the y axis when computing the byte offset into the bitmap.
                var byteIndex = ((ctx.height - y - 1) * ctx.width) * 4
                if y == ctx.height-1 || y % 10 == 0 {
                  let progress = (Double(y) / Double(ctx.height)) * 100.0
                  DispatchQueue.main.async {
                      self.progressIndicator?.doubleValue = progress
                  }
                }
                for x in 0..<ctx.width {
                    var color = blackColor
                    let step = mandelbrotStep(x: Double(x), y: Double(y), width: Double(ctx.width), height: Double(ctx.height))
                    if step < maxIterations {
                        let colorIndex = Int((Double(step) / Double(maxIterations)) * 100.0)
                        color = curColors[colorIndex]
                    }
                    if let data = ctx.data {
                        let ptr = data.assumingMemoryBound(to: UInt8.self)
                        ptr[byteIndex] = color[0]
                        ptr[byteIndex+1] = color[1]
                        ptr[byteIndex+2] = color[2]
                        ptr[byteIndex+3] = color[3]
                        byteIndex += 4
                    }
                }
            }
        }
    }
    
    func mandelbrotStep(x: Double, y: Double, width: Double, height: Double) -> Int {
        var cx: Double = 0.0
        var cy: Double = 0.0
        let (sx, sy) = convertPixelCoordinateToComplex(x: x, y: y, pixelWidth: width, pixelHeight: height)
        var iteration: Int = 0
        while (cx * cx + cy * cy <= 4.0 && iteration < self.maxIterations) {
            let xtemp = cx * cx - cy * cy + sx
            cy = 2.0 * cx * cy + sy
            cx = xtemp
            iteration += 1
        }
        return iteration
    }
    
    func renderSimd() {
        var simdx = SIMD8<Double>()
        guard let ctx = mandelContext else { return }
        guard let data = ctx.data else { return }
        let ptr = data.assumingMemoryBound(to: UInt8.self)
        
        let deltaX = curWidth / Double(ctx.width)
        let deltaY = curHeight / Double(ctx.height)
        for y in 0..<ctx.height {
            // Note that the byte ordering of the bitmap context in memory starts with pixel (0, 0) in the
            // upper left corner, whereas the MacOS pixel coordinate system has (0, 0) at the lower left corner.
            // So, we flip the y axis when computing the byte offset into the bitmap.
            var byteIndex = ((ctx.height - y - 1) * ctx.width) * 4
            if y == ctx.height-1 || y % 10 == 0 {
                let progress = (Double(y) / Double(ctx.height)) * 100.0
                DispatchQueue.main.async {
                    self.progressIndicator?.doubleValue = progress
                }
            }
            let cy = deltaY * Double(y) + curY
            let simdy = SIMD8<Double>(repeating: cy)
            var x: Int = 0
            while x < ctx.width {
                var cx = deltaX * Double(x) + curX
                for index in 0..<8 {
                    simdx[index] = cx
                    cx += deltaX
                }
                let steps = mandelbrotStepSimd(x: simdx, y: simdy)
                for index in 0..<8 {
                    var color = blackColor
                    let step = steps[index]
                    if step < 100.0 {
                        let colorIndex = Int(step)
                        color = curColors[colorIndex]
                    }
                    ptr[byteIndex] = color[0]
                    ptr[byteIndex+1] = color[1]
                    ptr[byteIndex+2] = color[2]
                    ptr[byteIndex+3] = color[3]
                    byteIndex += 4
                }
                x += 8
            }
        }
    }
    
    // SIMD implementation of Mandelbrot calculation. Returns a SIMD8 of values from 0->100.0 representing
    // color index for each pixel.
    // https://codereview.stackexchange.com/questions/221952/simd-mandelbrot-calculation
    func mandelbrotStepSimd(x: SIMD8<Double>, y: SIMD8<Double>) -> SIMD8<Double> {
        var cx = SIMD8<Double>.zero
        var cy = SIMD8<Double>.zero
        let thresholds = SIMD8<Double>(repeating: 4.0)
        let maxIter = Double(maxIterations)
        var notEscaped = SIMDMask<SIMD8<Double.SIMDMaskScalar>>(repeating: true)
        let isDone = SIMDMask<SIMD8<Double.SIMDMaskScalar>>(repeating: false)
        var currentIterations = 0.0
        var iterations = SIMD8<Double>.zero
        
        repeat {
            currentIterations += 1
            iterations.replace(with: currentIterations, where: notEscaped)
            let cxSquared = cx * cx
            let cySquared = cy * cy
            cy = 2.0 * cx * cy + y
            cx = cxSquared - cySquared + x
            notEscaped = cxSquared + cySquared .< thresholds
        } while notEscaped != isDone && currentIterations < maxIter
        
        iterations /= maxIter
        iterations *= 100.0
        iterations.replace(with: 100.0, where: notEscaped)
        return iterations
    }
    
    func convertPixelCoordinateToComplex(x: Double, y: Double, pixelWidth: Double, pixelHeight: Double) -> (Double, Double) {
        let mx = self.curWidth / pixelWidth
        let my = self.curHeight / pixelHeight
        return (mx * x + self.curX, my * y + self.curY)
    }
    
    func convertComplexToPixelCoordinate(cx: Double, cy: Double, pixelWidth: Double, pixelHeight: Double) -> (Double, Double) {
        let mx = pixelWidth / self.curWidth
        let bx = -mx * self.curX
        let my = pixelHeight / self.curHeight
        let by = -my * self.curY
        return (mx * cx + bx, my * cy + by)
    }
    
    // https://github.com/IsaacXen/Demo-Cocoa-Drag-Selection/blob/master/ViewsInFrame/ContainerView.swift
    private var downPoint: CGPoint?
    private var selectionRect: NSRect? = .zero {
        didSet {
            needsDisplay = true
        }
    }
    
    override func mouseDown(with event: NSEvent) {
        super.mouseDown(with: event)
        downPoint = convert(event.locationInWindow, from: nil)
    }
    
    override func mouseDragged(with event: NSEvent) {
        super.mouseDragged(with: event)
        let currentPoint = convert(event.locationInWindow, from: nil)
        selectionRect = frame.rect(from: downPoint!, to: currentPoint)
    }
    
    override func mouseUp(with event: NSEvent) {
        super.mouseUp(with: event)
        guard let selection = selectionRect else {
            return
        }
        let (cxMin, cyMin) = convertPixelCoordinateToComplex(x: Double(selection.minX), y: Double(selection.minY), pixelWidth: Double(self.frame.width), pixelHeight: Double(self.frame.height))
        let (cxMax, cyMax) = convertPixelCoordinateToComplex(x: Double(selection.maxX), y: Double(selection.maxY), pixelWidth: Double(self.frame.width), pixelHeight: Double(self.frame.height))
        self.curX = cxMin
        self.curY = cyMin
        let dim = max(cxMax - cxMin, cyMax - cyMin)
        self.curWidth = dim
        self.curHeight = dim
        doRender()
        
        selectionRect = nil
        downPoint = nil
    }
    
    private var scrollStart: CGPoint?
    private var scrollEnd: CGPoint? = .zero {
        didSet {
            needsDisplay = true
        }
    }
    override func scrollWheel(with event: NSEvent) {
        super.scrollWheel(with: event)
        if event.phase == .began {
            scrollStart = convert(event.locationInWindow, from: nil)
            scrollEnd = convert(event.locationInWindow, from: nil)
        } else if event.phase == .changed {
            scrollEnd!.x += event.deltaX
            if event.isDirectionInvertedFromDevice {
                scrollEnd!.y += event.deltaY
            } else {
                scrollEnd!.y -= event.deltaY
            }
        } else if event.phase == .ended {
            let (startX, startY) = convertPixelCoordinateToComplex(x: Double(scrollStart!.x), y: Double(scrollStart!.y), pixelWidth: Double(self.frame.width), pixelHeight: Double(self.frame.height))
            let (endX, endY) = convertPixelCoordinateToComplex(x: Double(scrollEnd!.x), y: Double(scrollEnd!.y), pixelWidth: Double(self.frame.width), pixelHeight: Double(self.frame.height))
            self.curX -= endX - startX
            self.curY -= endY - startY
            doRender()
            scrollStart = nil
            scrollEnd = nil
        }
    }
    
    override func magnify(with event: NSEvent) {
        super.magnify(with: event)
        print("magnify: got \(event)")
        if event.phase == .began {
            selectionRect = NSRect(x: 0, y: 0, width: self.frame.width, height: self.frame.height)
        } else if event.phase == .changed {
            let deltaWidth = event.magnification * self.frame.width
            let deltaHeight = event.magnification * self.frame.height
            selectionRect = NSRect(x: selectionRect!.minX + deltaWidth, y: selectionRect!.minY + deltaHeight, width: selectionRect!.width - (2 * deltaWidth), height: selectionRect!.height - (2 * deltaHeight))
            selectionRect!.insetBy(dx: 10, dy: 10)
        } else if event.phase == .ended {
            guard let selection = selectionRect else {
                return
            }
            let (cxMin, cyMin) = convertPixelCoordinateToComplex(x: Double(selection.minX), y: Double(selection.minY), pixelWidth: Double(self.frame.width), pixelHeight: Double(self.frame.height))
            let (cxMax, cyMax) = convertPixelCoordinateToComplex(x: Double(selection.maxX), y: Double(selection.maxY), pixelWidth: Double(self.frame.width), pixelHeight: Double(self.frame.height))
            self.curX = cxMin
            self.curY = cyMin
            let dim = max(cxMax - cxMin, cyMax - cyMin)
            self.curWidth = dim
            self.curHeight = dim
            doRender()
            
            selectionRect = nil
        }
    }

    @IBAction func handleIterationSliderChange(_ sender: Any) {
        iterationsTextField.intValue = iterationsSlider.intValue
        maxIterations = UInt32(iterationsSlider.intValue)
        doRender()
    }
    
    @IBAction func handleIterationsTextFieldChange(_ sender: Any) {
        iterationsSlider.intValue = iterationsTextField.intValue
        maxIterations = UInt32(iterationsTextField.intValue)
        doRender()
    }
    
    @IBAction func handleHomeButtonClicked(_ sender: Any) {
        self.curX = -2.0
        self.curY = -1.5
        self.curWidth = 3.0
        self.curHeight = 3.0
        self.maxIterations = 100
        iterationsTextField.intValue = 100
        iterationsSlider.intValue = 100
        doRender()
    }
}

extension NSRect {
    func rect(from point1: CGPoint, to point2: CGPoint) -> NSRect {
        let x = max(min(point1.x, point2.x), 0)
        let y = max(min(point1.y, point2.y), 0)
        let w = abs(min(max(0, point1.x), width) - min(max(0, point2.x), width))
        let h = abs(min(max(0, point1.y), height) - min(max(0, point2.y), height))
        return NSMakeRect(x, y, w, h)
    }
}
