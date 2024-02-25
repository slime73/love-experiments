print("abc")

function love.conf(t)
  print("def")
  t.console = true
  t.window.name = 'love.test'
  t.window.width = 360
  t.window.height = 240
  t.window.resizable = true
  t.window.depth = true
  t.window.stencil = true
  t.window = nil
  t.modules.graphics = false
  t.modules.window = false
  t.modules.joystick = false
  t.modules.audio = false
  t.renderers = {"opengl"}
end
