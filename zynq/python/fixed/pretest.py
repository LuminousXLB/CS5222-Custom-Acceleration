from distutils.log import error
import numpy as np
import matplotlib.pyplot as plt


if __name__ == "__main__":

    coef_ = np.load("model_weights.npy")
    intercept_ = np.load("model_offsets.npy")

    test_data = np.load("test_data.npy").astype(np.uint8)
    test_labels = np.load("test_labels.npy").astype(np.int32)

    scales = []
    error_rate = []

    for i in range(0, 32):
        SCALE = 1 << i

        offset = np.clip(intercept_ * SCALE, -128, 127).astype(np.int32)
        weight = np.clip(coef_ * SCALE, -128, 127).astype(np.int8)

        ones = np.ones(len(test_data)).reshape((len(test_data), 1))
        i_p = np.append(ones, test_data, axis=1)
        w_p = np.append(offset.reshape(10, 1), weight, axis=1)
        fixed_labels = np.dot(i_p, w_p.T)

        fixed_errors = 0
        for idx, label in enumerate(test_labels):
            guess_label = np.argmax(fixed_labels[idx])
            actual_label = np.argmax(label)
            if guess_label != actual_label:
                fixed_errors += 1.0

        scales.append(SCALE)
        error_rate.append(fixed_errors / len(test_labels) * 100)

    fig, ax = plt.subplots(figsize=(8, 5), dpi=120)

    ax.scatter(scales, error_rate)
    ax.axhline(y=20, linestyle="dashed", color="orange")

    ax.set_xscale("log", base=2)
    ax.set_xlabel("Scale")
    ax.set_ylabel("Error Rate (%)")
    ax.set_xlim(2**0, 2**32)
    ax.set_ylim(0, 100)

    fig.savefig("scale_pretest.pdf")

    # offset = np.load("model_offsets_fixed.npy").astype(np.int32)
    # weight = np.load("model_weights_fixed.npy").astype(np.int8)
