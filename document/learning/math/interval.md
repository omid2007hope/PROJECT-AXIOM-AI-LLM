In math, an **interval** is simply a continuous set of numbers between two endpoints. In code, it is just a `range`, a slice, or a clamped bounding box between a minimum and maximum value.

The only confusing part about intervals in math literature is the bracket notation, which strictly defines whether the endpoints are included or excluded.

---

### The Bracket Notation Cheat Sheet

Mathematicians use square brackets `[` for **inclusive** bounds and parentheses `(` for **exclusive** bounds:

| Mathematical Notation | Name | Mathematical Condition | Code Equivalent |
| --- | --- | --- | --- |
| **$[a, b]$** | Closed interval | $a \le x \le b$ | `x >= a && x <= b` |
| **$(a, b)$** | Open interval | $a < x < b$ | `x > a && x < b` |
| **$[a, b)$** | Half-open (left-closed, right-open) | $a \le x < b$ | `x >= a && x < b` |
| **$(a, b]$** | Half-open (left-open, right-closed) | $a < x \le b$ | `x > a && x <= b` |
| **$[0, \infty)$** | Unbounded / Ray | $x \ge 0$ | `x >= 0` |
| **$(-\infty, \infty)$** | Real line ($\mathbb{R}$) | $-\infty < x < \infty$ | Any float value |

---

### Why Computer Science Loves Half-Open Intervals $[a, b)$

In programming, half-open intervals $[a, b)$ are the universal default (e.g., C++ iterators `[begin, end)`, Python `range(0, 10)`, or raw pointer offsets `ptr` to `ptr + size`).

Math papers often drop into closed intervals $[a, b]$, but half-open intervals are favored in implementation for three practical reasons:

1. **Clean Length Calculation:** The number of elements (or span length) is strictly $b - a$. With $[0, 10)$, the length is $10 - 0 = 10$. With closed $[0, 10]$, the length is $10 - 0 + 1 = 11$.
2. **Adjacent Splitting without Overlap:** Two adjacent chunks connect cleanly at the boundary:

$$[0, 5) \cup [5, 10) = [0, 10)$$



There is no duplicate checking at boundary index `5`.
3. **Empty Sets are Trivial:** If $a = b$, $[a, a)$ is an empty interval (length `0`), whereas $[a, a]$ contains the point $a$.

---

### How Intervals Show Up in AI & Machine Learning

When reading model architectures or optimization papers, intervals define the **domain** (valid inputs) and **codomain / range** (possible outputs).

#### 1. Activation Function Ranges

Activation functions squash unbounded linear activations $(-\infty, \infty)$ into bounded intervals to stabilize numerical gradients:

* **Sigmoid:** Maps $(-\infty, \infty) \to (0, 1)$
Used for binary probabilities where the output must live strictly inside the open interval $(0, 1)$ to avoid taking $\log(0)$ in loss functions.
* **Tanh:** Maps $(-\infty, \infty) \to (-1, 1)$
Zero-centered outputs bounded symmetrically around zero.
* **ReLU:** Maps $(-\infty, \infty) \to [0, \infty)$
Left-clamped at 0 (closed bound), but unbounded on the positive side.

#### 2. Min-Max Normalization (Interval Remapping)

Preparing raw data for neural networks is just transforming an arbitrary data interval $[x_{\min}, x_{\max}]$ into a target uniform interval like $[0, 1]$ or $[-1, 1]$:

$$x_{\text{norm}} = \frac{x - x_{\min}}{x_{\max} - x_{\min}}$$

```cpp
float normalize(float x, float min_val, float max_val) {
    return (x - min_val) / (max_val - min_val); // re-maps [min, max] -> [0, 1]
}

```

#### 3. Gradient Clipping

To prevent exploding gradients during backpropagation, training code clamps the gradient vector into a fixed interval $[-c, c]$:

```cpp
float clip(float g, float c) {
    if (g > c) return c;
    if (g < -c) return -c;
    return g; // g is now constrained to [-c, c]
}

```

Whenever a paper says "let $x \in [0, 1]$", it simply means: "$x$ is a variable clamped between 0 and 1, inclusive."