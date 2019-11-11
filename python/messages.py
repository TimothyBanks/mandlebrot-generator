import io
import StringIO

class Message_header:
    # def __init__(self, type: int, identifier: str):
    def __init__(self, type, identifier):
        self.type = type
        self.identifier = identifier

    @classmethod
    def from_stream(cls, stream):
        type = int(stream.readline())
        identifier = stream.readline()[:-1]
        return Message_header(type, identifier)

    def clone(self):
        buffer = repr(self)
        stream = StringIO.StringIO(buffer)
        return Message_header.from_stream(stream)

    def __eq__(self, other):
        if self.type != other.type:
            return False

        if self.identifier != other.identifier:
            return False

        return True

    def __ne__(self, other):
        return not self.__eq__(self, other)

    def __str__(self):
        result = 'Type: ' + str(self.type) + '\n'
        result += 'Identifier: ' + self.identifier + '\n'
        return result

    def __repr__(self):
        result = repr(self.type) + '\n'
        result += self.identifier + '\n'
        return result

class View:
    def __init__(self, left, top, right, bottom):
        self.left = left
        self.top = top
        self.right = right
        self.bottom = bottom

    @classmethod
    def from_stream(cls, stream):
        left = float(stream.readline())
        top = float(stream.readline())
        right = float(stream.readline())
        bottom = float(stream.readline())
        return View(left, top, right, bottom)

    def clone(self):
        buffer = repr(self)
        stream = StringIO.StringIO(buffer)
        return View.from_stream(stream)

    def __eq__(self, other):
        if self.left != other.left:
            return False

        if self.top != other.top:
            return False

        if self.right != other.right:
            return False

        if self.bottom != other.bottom:
            return False

        return True

    def __ne__(self, other):
        return not self.__eq__(self, other)

    def __str__(self):
        result = 'Left: ' + str(self.left) + '\n'
        result += 'Top: ' + str(self.top) + '\n'
        result += 'Right: ' + str(self.right) + '\n'
        result += 'Bottom: ' + str(self.bottom) + '\n'
        return result

    def __repr__(self):
        result = repr(self.left) + '\n'
        result += repr(self.top) + '\n'
        result += repr(self.right) + '\n'
        result += repr(self.bottom) + '\n'
        return result

    def width(self):
        return abs(self.right - self.left)

    def height(self):
        return abs(self.bottom - self.top)

class Geo_message_header:
    # def __init__(self, header: Message_header, pixel_view: View, complex_view: View):
    def __init__(self, header, pixel_view, complex_view):
        self.header = header
        self.pixel_view = pixel_view
        self.complex_view = complex_view

    @classmethod
    def from_stream(cls, stream):
        header = Message_header.from_stream(stream)
        pixel_view = View.from_stream(stream)
        complex_view = View.from_stream(stream)
        return Geo_message_header(header, pixel_view, complex_view)

    def clone(self):
        buffer = repr(self)
        stream = StringIO.StringIO(buffer)
        return Geo_message_header.from_stream(stream)

    def __eq__(self, other):
        if self.header != other.header:
            return False

        if self.pixel_view != other.pixel_view:
            return False

        if self.complex_view != other.complex_view:
            return False

        return True

    def __ne__(self, other):
        return not self.__eq__(self, other)

    def __str__(self):
        result = str(self.header) + '\n'
        result += 'Pixel View: ' + str(self.pixel_view) + '\n'
        result += 'Complex View: ' + str(self.complex_view) + '\n'
        return result

    def __repr__(self):
        result = repr(self.header)
        result += repr(self.pixel_view)
        result += repr(self.complex_view)
        return result

class Request_message:
    ID = 0

    # def __init__(self, header: Geo_message_header, max_interations: int):
    def __init__(self, header, max_interations):
        self.header = header
        self.max_iterations = max_interations

    @classmethod
    def from_stream(cls, stream):
        header = Geo_message_header.from_stream(stream)
        max_iterations = int(stream.readline())
        return Request_message(header, max_iterations)

    def clone(self):
        buffer = repr(self)
        stream = StringIO.StringIO(buffer)
        return Request_message.from_stream(stream)

    def __eq__(self, other):
        if self.header != other.header:
            return False

        if self.max_iterations != other.max_iterations:
            return False

        return True

    def __ne__(self, other):
        return not self.__eq__(self, other)

    def __str__(self):
        result = str(self.header)
        result += 'Max Iterations: ' + str(self.max_iterations) + '\n'
        return result

    def __repr__(self):
        result = repr(self.header)
        result += repr(self.max_iterations) + '\n'
        return result

class ARGB_buffer:
    # def __init__(self, argb_buffer: list, width: int, height: int):
    def __init__(self, argb_buffer, width, height):
        self.argb_buffer = argb_buffer
        self.width = width
        self.height = height

    @classmethod
    def from_stream(cls, stream, width, height):
        argb_buffer = []
        for i in range(int(height)):
            factor = i * width
            for j in range(int(width)):
                argb_buffer.append(int(stream.readline()))
        return ARGB_buffer(argb_buffer, width, height)

    def clone(self):
        buffer = repr(self)
        stream = StringIO.StringIO(buffer)
        return ARGB_buffer.from_stream(stream)

    def __eq__(self, other):
        if self.argb_buffer != other.argb_buffer:
            return False

        if self.width != other.width:
            return False

        if self.height != other.height:
            return False

        return True

    def __ne__(self, other):
        return not self.__eq__(self, other)

    def __str__(self):
        count = 0

        result = 'P3\n' + str(int(self.width)) + ' ' + str(int(self.height)) + ' ' + '255\n'
        for i in range(int(self.height) - 1, -1, -1):
            factor = i * self.width
            for j in range(int(self.width)):
                count += 1
                color = int(self.argb_buffer[int(j + factor)])
                result += str((color >> 16) & 0xFF) + ' '
                result += str((color >> 8) & 0xFF) + ' '
                result += str(color & 0xFF) + '\n'

        expected = self.width * self.height

        return result

    def __repr__(self):
        result = ''
        for i in range(int(self.height)):
            factor = i * self.width
            for j in range(int(self.width)):
                color = self.argb_buffer[int(j + factor)]
                result += str(color) + '\n'

        return result

class Response_message:
    ID = 1

    # def __init__(self, header: Geo_message_header, argb_buffer):
    def __init__(self, header, argb_buffer):
        self.header = header
        self.argb_buffer = argb_buffer

    @classmethod
    def from_stream(cls, stream):
        header = Geo_message_header.from_stream(stream)
        argb_buffer = ARGB_buffer.from_stream(stream, header.pixel_view.width(), header.pixel_view.height())
        return Response_message(header, argb_buffer)

    def clone(self):
        buffer = repr(self)
        stream = StringIO.StringIO(buffer)
        return Response_message.from_stream(stream)

    def __eq__(self, other):
        if self.header != other.header:
            return False

        if self.argb_buffer != other.argb_buffer:
            return False

        return True

    def __ne__(self, other):
        return not self.__eq__(self, other)

    def __str__(self):
        result = str(self.header)
        result += str(self.argb_buffer)
        return result

    def __repr__(self):
        result = repr(self.header)
        result += repr(self.argb_buffer)
        return result

class Cancel_message:
    ID = 2

    # def __init__(self, header: Message_header):
    def __init__(self, header):
        self.header = header

    @classmethod
    def from_stream(cls, stream):
        header = Message_header.from_stream(stream)
        return Cancel_message(header)

    def clone(self):
        buffer = repr(self)
        stream = StringIO.StringIO(buffer)
        return Cancel_message.from_stream(stream)

    def __eq__(self, other):
        if self.header != other.header:
            return False

        return True

    def __ne__(self, other):
        return not self.__eq__(self, other)

    def __str__(self):
        result = str(self.header)
        return result

    def __repr__(self):
        result = repr(self.header)
        return result
