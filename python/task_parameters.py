import image_view as Image_view
import messages as Messages

class Token:
    def __init__(self):
        self.value = False

class Task_parameters:
    # def __init__(self, identifier: str, cancel_token: Token, on_task_completed_callback, on_task_canceled_callback):
    def __init__(self, identifier, cancel_token, on_task_completed_callback, on_task_canceled_callback):
        self.identifier = identifier
        self.cancel_token = cancel_token
        self.on_task_completed_callback = on_task_completed_callback
        self.on_task_canceled_callback = on_task_canceled_callback

class Generator_task_parameters(Task_parameters):
    # def __init__(self,
    #             identifier: str,
    #              fractal_view: Messages.View,
    #              tile_pixel_view: Messages.View,
    #              tile_complex_view: Messages.View,
    #              cancel_token: Token,
    #              on_task_completed_callback,
    #             on_task_canceled_callback):
    def __init__(self,
                identifier,
                fractal_view,
                tile_pixel_view,
                tile_complex_view,
                cancel_token,
                on_task_completed_callback,
                on_task_canceled_callback):
        Task_parameters.__init__(self, identifier, cancel_token, on_task_completed_callback, on_task_canceled_callback)
        self.fractal_view = fractal_view
        self.tile_pixel_view = tile_pixel_view
        self.tile_complex_view = tile_complex_view

    @classmethod
#    def distribute(cls,
#                   identifier: str,
#                   cancel_token: Token,
#                   on_task_completed_callback,
#                   on_task_canceled_callback,
#                   fractal_view: Messages.View,
#                   tile_width: int,
#                   tile_height: int):
    def distribute(cls,
                   identifier,
                   cancel_token,
                   on_task_completed_callback,
                   on_task_canceled_callback,
                   fractal_view,
                   tile_width,
                   tile_height):
        tasks = []

        if (tile_height <= 0
                or tile_width <= 0
                or tile_height >= fractal_view.pixel_view.height()
                or tile_width >= fractal_view.pixel_view.width()):
            tasks.append(Generator_task_parameters(identifier,
                                                   fractal_view,
                                                   fractal_view.pixel_view,
                                                   fractal_view.complex_view,
                                                   cancel_token,
                                                   on_task_completed_callback,
                                                   on_task_canceled_callback))
            return tasks

        rows = int(fractal_view.pixel_view.height() / tile_height)
        if rows * tile_height != fractal_view.pixel_view.height():
            rows += 1

        cols = int(fractal_view.pixel_view.width() / tile_width)
        if cols * tile_width != fractal_view.pixel_view.width():
            cols += 1

        complex_width_interval = fractal_view.complex_view.width() / float(cols)
        complex_height_interval = fractal_view.complex_view.height() / float(rows)

        for i in range(int(cols)):
            for j in range(int(rows)):
                pixel_view = Messages.View(fractal_view.pixel_view.left + i * tile_width,
                                           fractal_view.pixel_view.top + j * tile_height,
                                           fractal_view.pixel_view.right if i == cols - 1 else fractal_view.pixel_view.left + (i + 1) * tile_width,
                                           fractal_view.pixel_view.bottom if j == rows - 1 else fractal_view.pixel_view.top + (j + 1) * tile_height)
                complex_view = Messages.View(fractal_view.complex_view.left + i * complex_width_interval,
                                             fractal_view.complex_view.top + j * complex_height_interval,
                                             fractal_view.complex_view.right if i == cols - 1 else fractal_view.complex_view.left + (i + 1) * complex_width_interval,
                                             fractal_view.complex_view.bottom if j == rows - 1 else fractal_view.complex_view.top + (j + 1) * complex_height_interval)
                tasks.append(Generator_task_parameters(identifier,
                                                       fractal_view,
                                                       pixel_view,
                                                       complex_view,
                                                       cancel_token,
                                                       on_task_completed_callback,
                                                       on_task_canceled_callback))

        return tasks