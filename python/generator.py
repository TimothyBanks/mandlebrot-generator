import cmath
import image_view as Image_view
import messages as Messages
import task_parameters as Task_parameters

class Generator:
    def invoke(self, function, task_parameters):
        for task in task_parameters:
            real_factor = task.fractal_view.complex_view.width() / task.fractal_view.pixel_view.width()
            imaginary_factor = task.fractal_view.complex_view.height() / task.fractal_view.pixel_view.height()

            for i in range(int(task.tile_pixel_view.left), int(task.tile_pixel_view.right), 1):
                for j in range(int(task.tile_pixel_view.top), int(task.tile_pixel_view.bottom), 1):
                    c = complex(task.fractal_view.complex_view.left + i * real_factor, task.fractal_view.complex_view.top + j * imaginary_factor)
                    task.fractal_view.buffer.argb_buffer[i + j * int(task.fractal_view.pixel_view.width())] = function.invoke(c, task.cancel_token)

                    if task.cancel_token is True:
                        break

                if task.cancel_token is True:
                    break

            if task.cancel_token is True and task.on_task_canceled_callback != None:
                task.on_task_canceled_callback(task)
                break

            if task.on_task_completed_callback != None:
                task.on_task_completed_callback(task)

