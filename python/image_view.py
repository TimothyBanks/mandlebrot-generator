import messages as Messages

class Image_view:
    # def __init__(self, pixel_view: Messages.View, complex_view: Messages.View, buffer: Messages.ARGB_buffer):
    def __init__(self, pixel_view, complex_view, buffer):
        self.pixel_view = pixel_view
        self.complex_view = complex_view
        self.buffer = buffer
