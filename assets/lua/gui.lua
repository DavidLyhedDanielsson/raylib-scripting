if single == nil then
    single = 1
    double = { 1, 2 }
    triple = { 1, 2, 3 }
    quad = { 1, 2, 3, 4 }
    window_open = true
    radio_val = 0
    range_min = 5
    range_max = 10
    color = {1, 1, 1, 1}
end

Begin("Lua window", window_open)

-- pos = GetWindowPos()

BeginChild("ChildArea", {x = 0, y = 100})

Button("Hello there!")
Button("Hello there!")
Button("Hello there!")
Button("Hello there!")
SmallButton("Another hello")
ArrowButton("Left", 0)
if BeginCombo("Dropdown", "preview", 0) then
    EndCombo()
end
EndChild()

_, single = DragFloat("Single", single, 2.5, 0, 50)
_, double = DragFloat2("Double", double, 2.5, 0, 50)
_, triple = DragFloat3("Triple", triple, 2.5, 0, 50)
_, quad = DragFloat4("Quad", quad, 2.5, 0, 50)

_, range_min, range_max = DragFloatRange2("Range", range_min, range_max, 1.0, 0, 100)

_, radio_val = RadioButtonMult("First", radio_val, 0)
_, radio_val = RadioButtonMult("Second", radio_val, 1)
_, radio_val = RadioButtonMult("Third", radio_val, 2)

_, color = ColorPicker4("Color", color);

Text("Hello")
TextColored({x = 1.0, y = 0.5, z = 0.1, w = 1.0}, "Hello")
TextDisabled("Noooo")
TextWrapped("Wroawroicawh rjlcjahwrljcawh rlicujks chflkajseahcf jkelh asjelkehfla slkjh alskjefh akcslejfh lkj")
LabelText("Label", "And value")
BulletText("Completed")

End()