import cmath
import generator as Generator
import image_view as Image_view
import io
import StringIO
import mandlebrot_function as Mandlebrot
import messages as Messages
import task_parameters as Task_parameters

def main():
    pixel_view = Messages.View(0, 0, 256, 256)

    min_real = -2.0
    max_real = 2.0
    min_imaginary = -2.0
    max_imaginary = min_imaginary + (max_real - min_real) * (float(pixel_view.height()) / float(pixel_view.width()))
    complex_view = Messages.View(min_real, min_imaginary, max_real, max_imaginary)

    buffer = list(range(pixel_view.width() * pixel_view.height()))
    argb_buffer = Messages.ARGB_buffer(buffer, pixel_view.width(), pixel_view.height())

    header = Messages.Message_header(Messages.Request_message.ID, '0')
    geo_header = Messages.Geo_message_header(header, pixel_view, complex_view)
    message = Messages.Request_message(geo_header, 512)
    string_view = str(message)
    repr_view = repr(message)
    print(repr_view)
    stream = StringIO.StringIO(repr_view)
    message_2 = Messages.Request_message.from_stream(stream)

    header = Messages.Message_header(Messages.Response_message.ID, '1')
    geo_header = Messages.Geo_message_header(header, pixel_view, complex_view)
    message = Messages.Response_message(geo_header, argb_buffer)
    string_view = str(message)
    repr_view = repr(message)
    stream = StringIO.StringIO(repr_view)
    message_2 = Messages.Response_message.from_stream(stream)

    header = Messages.Message_header(Messages.Cancel_message.ID, '2')
    message = Messages.Cancel_message(header)
    string_view = str(message)
    repr_view = repr(message)
    stream = StringIO.StringIO(repr_view)
    message_2 = Messages.Cancel_message.from_stream(stream)

    pixel_view = Messages.View(0, 0, 1463, 1315)
    argb_buffer = Messages.ARGB_buffer([0XFFFFFFFF] * pixel_view.width() * pixel_view.height(), pixel_view.width(), pixel_view.height())
    fractal_view = Image_view.Image_view(pixel_view, complex_view, argb_buffer)
    function = Mandlebrot.Mandlebrot(1000)
    cancel_token = Task_parameters.Token()

    tasks = Task_parameters.Generator_task_parameters.distribute("1",
                                                                 cancel_token,
                                                                 None,
                                                                 None,
                                                                 fractal_view,
                                                                 64,
                                                                 64)
    generator = Generator.Generator()
    generator.invoke(function, tasks)

    file_contents = str(argb_buffer)
    file = open("mandlebrot.ppm", "w")
    file.write(file_contents)
    file.close()

if __name__ == '__main__':
    main()
