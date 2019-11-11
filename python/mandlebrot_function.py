import cmath
import task_parameters as Task_parameters

class Mandlebrot:
    # def __init__(self, max_iterations: int):
    def __init__(self, max_iterations):
        self.max_iterations = max_iterations

    # def invoke(self, z, cancel_token: Task_parameters.Token):
    def invoke(self, z, cancel_token):
        BLACK = 0x00000000

        c = complex(z.real, z.imag)
        iterations = self.max_iterations
        for i in range(iterations):
            if z.real * z.real + z.imag * z.imag > 4:
                t = float(i) / float(iterations)
                r = int(9 * (1 - t) * t * t * t * 255)
                g = int(15 * (1 - t) * (1 - t) * t * t * 255)
                b = int(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255)
                return (0xff << 24) | (r << 16) | (g << 8) | b

            z = z * z + c

            if cancel_token.value is True:
                return BLACK

        return BLACK
