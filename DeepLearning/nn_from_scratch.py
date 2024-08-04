import numpy as np
import random
np.seterr(all='ignore')

# Utilize numpy for vectorized operations of sigmoid and its derivative
def sigmoid(x):
    return 1 / (1 + np.exp(-x))

def dsigmoid(y):
    return y * (1.0 - y)

# Use numpy for tanh and its derivative
def tanh(x):
    return np.tanh(x)

def dtanh(y):
    return 1 - y**2

class MLP_NeuralNetwork:
    """
    Basic MultiLayer Perceptron (MLP) neural network model
    """
    def __init__(self, input_size, hidden_size, output_size, iterations, learning_rate, momentum, rate_decay):
        self.iterations = iterations
        self.learning_rate = learning_rate
        self.momentum = momentum
        self.rate_decay = rate_decay

        # Initialize layers with bias
        self.input_size = input_size + 1  # adding a bias node to input layer
        self.hidden_size = hidden_size
        self.output_size = output_size

        # Initialize activations
        self.ai = np.ones(self.input_size)
        self.ah = np.ones(self.hidden_size)
        self.ao = np.ones(self.output_size)

        # Weight initialization
        input_range = 1.0 / np.sqrt(self.input_size)
        output_range = 1.0 / np.sqrt(self.hidden_size)
        self.wi = np.random.normal(loc=0, scale=input_range, size=(self.input_size, self.hidden_size))
        self.wo = np.random.normal(loc=0, scale=output_range, size=(self.hidden_size, self.output_size))

        # Change matrices
        self.ci = np.zeros_like(self.wi)
        self.co = np.zeros_like(self.wo)

    def feedForward(self, inputs):
        if len(inputs) != self.input_size - 1:
            raise ValueError('Incorrect number of inputs')

        # Input activations
        self.ai[:-1] = inputs  # -1 to skip the bias

        # Hidden activations
        sum_h = np.dot(self.ai, self.wi)
        self.ah = tanh(sum_h)

        # Output activations
        sum_o = np.dot(self.ah, self.wo)
        self.ao = sigmoid(sum_o)

        return self.ao

    def backPropagate(self, targets):
        if len(targets) != self.output_size:
            raise ValueError('Incorrect number of target values')

        # Error terms for output
        output_deltas = dsigmoid(self.ao) * -(targets - self.ao)

        # Error terms for hidden
        error = np.dot(self.wo, output_deltas)
        hidden_deltas = dtanh(self.ah) * error

        # Update output weights
        self.wo -= self.learning_rate * np.outer(self.ah, output_deltas) + self.co * self.momentum
        self.co = np.outer(self.ah, output_deltas)

        # Update input weights
        self.wi -= self.learning_rate * np.outer(self.ai, hidden_deltas) + self.ci * self.momentum
        self.ci = np.outer(self.ai, hidden_deltas)

        # Calculate error
        error = 0.5 * np.sum((targets - self.ao) ** 2)
        return error

    def train(self, patterns):
        for i in range(self.iterations):
            error = 0.0
            random.shuffle(patterns)
            for inputs, targets in patterns:
                self.feedForward(inputs)
                error += self.backPropagate(targets)
            with open('error.txt', 'a') as errorfile:
                errorfile.write(f'{error}\n')
            if i % 10 == 0:
                print(f'Error {error:.5f}')
            self.learning_rate *= (self.learning_rate / (self.learning_rate + (self.learning_rate * self.rate_decay)))

    def test(self, patterns):
        for inputs, targets in patterns:
            print(f'{targets} -> {self.feedForward(inputs)}')

def load_data():
    data = np.loadtxt('Data/sklearn_digits.csv', delimiter=',')
    y = data[:, :10]
    x = (data[:, 10:] - data[:, 10:].min(axis=0)) / data[:, 10:].max(axis=0)
    return list(zip(x, y))

def demo():
    data = load_data()
    nn = MLP_NeuralNetwork(64, 100, 10, iterations=50, learning_rate=0.5, momentum=0.5, rate_decay=0.01)
    nn.train(data)
    nn.test(data)

if __name__ == '__main__':
    demo()
