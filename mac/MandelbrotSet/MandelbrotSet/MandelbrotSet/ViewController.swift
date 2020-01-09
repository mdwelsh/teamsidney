//
//  ViewController.swift
//  MandelbrotSet
//
//  Created by Matt Welsh on 1/2/20.
//  Copyright © 2020 Team Sidney Enterprises. All rights reserved.
//

import Cocoa

class ViewController: NSViewController {
    
    @IBOutlet weak var palettePopup: NSPopUpButton!
    @IBOutlet weak var mandelView: MandelView!
    @IBOutlet weak var iterationsSlider: NSSlider!
    @IBOutlet weak var iterationsTextField: NSTextField!
    
    override func viewDidLoad() {
        super.viewDidLoad()
    }
    
    override func viewWillAppear() {
        super.viewWillAppear()
        setupUI()
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
    
    func setupUI() {
        palettePopup.removeAllItems()
        palettePopup.addItem(withTitle: "Xray")
        palettePopup.addItem(withTitle: "Rainbow")
        palettePopup.addItem(withTitle: "Blue")
        palettePopup.addItem(withTitle: "Red")
        palettePopup.selectItem(withTitle: "Rainbow")
        mandelView.setPalette("Rainbow")
        
        if let window = mandelView.window {
            var frame = window.frame
            frame.size = NSMakeSize(800, 800)
            window.setFrame(frame, display: true)
        }
        
        iterationsSlider.intValue = 100
        iterationsTextField.intValue = iterationsSlider.intValue
    }
    

}

