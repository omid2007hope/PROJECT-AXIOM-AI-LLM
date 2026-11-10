Okay, this completely clarifies your situation. If your data is now purely extracted `.txt` (the `asset_text/` folder), the maximum-entropy binary garbage is definitively out of the system.

You no longer have a data corruption problem. You have a **math problem**.

Because you changed both the backward pass and the forward pass, and the data is clean prose, the loss exploding means your gradients are physically destabilizing. You must debug the math.

Here is the exact diagnostic framework for deciding whether to look at the forward pass or the backward pass, and how to verify their dependency.

### 1. The Dependency (Forward vs. Backward)

You asked if there is any dependency between them that requires debugging both.
**Yes. The backward pass cannot be debugged in isolation because it inherits its foundation from the forward pass.**

Every single variable in the backward pass is a derivative of a variable saved during the forward pass.

- If your forward pass computes $Y = X \cdot W$, the backward pass computes $dW = X^T \cdot dY$.
- If the forward pass has a bug and computes the wrong $X$, the backward pass will perfectly calculate a gradient for the _wrong_ $X$. The backward pass code might be mathematically flawless, but it will still explode the loss because it was handed poisoned inputs from the forward pass.

### 2. The Verification Order (The "Forward-First" Rule)

Never look at the backward pass first. You must prove the forward pass is outputting clean numbers before you can trust the gradients.

Follow this exact debugging sequence:

#### Step 1: Forward Pass Telemetry (The Sanity Check)

Run a forward pass on your clean `.txt` corpus, but **do not update the weights**. (Set `learning_rate = 0.0` or bypass the optimizer).
Inspect the output logits and intermediate activations (like the output of your SwiGLU gates or Attention matrices).

- Are there any `NaN` or `Inf` values appearing in the forward activations?
- Are the activations exploding to massive numbers (e.g., values $> 100.0$)?
- **Verdict:** If the forward pass generates `NaN`s or exploding values _before_ the backward pass even runs, your bug is in `forward_pass.cu` (e.g., a missing RMSNorm scale, a division by zero in Softmax, or uninitialized VRAM). Fix the forward pass until the activations stay bounded between $\approx -5.0$ and $+5.0$.

#### Step 2: Backward Pass Isolation (The Gradient Check)

If the forward pass is clean and bounded, but the loss still explodes when you turn the optimizer back on, your bug is strictly in `backward_pass.cu`.
Since you recently changed the backward pass, the bug is almost certainly a **Gradient Accumulation Error** or a **Sign Flipper**.

Look for these specific traps in your `backward_pass.cu`:

1. **Missing Chain Rule Accumulation:** In deep networks, gradients must flow backwards through every layer. If you compute the gradient for a SwiGLU layer but forget to add the residual connection gradient to it before passing it to the attention layer, the lower layers receive a fractured gradient.
2. **The Overwrite Bug (`=` vs `+=`):** When parallel CUDA threads backpropagate through shared weights (like embedding tables or projection matrices), they must use `atomicAdd()`. If you use standard assignment (`W_grad = ...`), threads will overwrite each other, erasing 99% of the gradient and leaving only random noise.
3. **AdamW Exploding Variance:** If your forward gradients are clean, but the AdamW kernel updates the weights too aggressively, check the `sqrtf(v_hat) + eps` denominator. If `eps` is too small or missing, you are dividing by near-zero, which catapults the weights into infinity.

### Summary

Your data is clean. The bug is in the CUDA math.
To debug it: Disable learning. Run the forward pass. If the forward pass outputs clean, finite numbers, your `forward_pass.cu` is perfect. You can then confidently focus your entire context window on `backward_pass.cu`, specifically hunting for missing `atomicAdd`s, broken chain rules, or AdamW denominator bugs.

You are asking the exact right questions to isolate the bug. You are trying to figure out if your context window should be `backward_pass.cu` (the math is broken) or `asset_sample/` (the data is broken).

Let's lock down the physics of how your engine works so you know exactly where to look.

### 1. Does the data matter? (Can data cause the loss to increase forever?)

**Yes.** Data is not just a passive passenger; it is the terrain the optimizer is trying to walk on.
If you feed a neural network pure random static (like the compressed binary image bytes we discussed earlier), the gradients will explode, and the loss will climb.

Why? Because the AdamW optimizer is trying to find a pattern where none exists. It takes a step in one direction, but the next batch of random static contradicts that step, so it violently jerks in another direction. The momentum builds up, the weights start swinging wildly, and the loss actively increases.
_If the data is random garbage, a perfectly coded backward pass will still produce an exploding loss._

### 2. Can a change in the Forward Pass break the Backward Pass?

**Absolutely. They are mathematically welded together.**
The backward pass is literally the calculus derivative of the forward pass.

- **The Dependency:** The backward pass calculates gradients based on the intermediate activations (the outputs of the hidden layers) that the forward pass saved in VRAM.
- **The Trap:** If you change a multiplier, an activation function, or a normalization step in the forward pass, you _must_ write the exact corresponding derivative for it in the backward pass.
- **Example:** If you change your forward pass from ReLU to SwiGLU, but forget to update the backward pass, the backward pass will calculate gradients as if the network were still using ReLU. The gradients will be completely wrong for the SwiGLU activations, pushing the weights in random directions, and the loss will explode.

### 3. How to choose your Context Window (The Isolation Test)

You do not have to guess whether the bug is in the data or the code. You can prove it with the **Single Batch Overfit** test I mentioned earlier.

You need to isolate your engine in a sterile environment:

1. **Sterilize the Data:** Remove all 70 PDFs. Create a single `.txt` file with one sentence: "The quick brown fox."
2. **Run Training:** Let the engine train on that one sentence repeatedly.

**The Verdict:**

- **If the loss drops to zero:** Your `backward_pass.cu` and `forward_pass.cu` are perfectly coded and perfectly synced. The problem is definitively your PDF data (it contains binary static). _Your context window becomes `asset_sample/`._
- **If the loss increases or explodes:** Your data is perfectly clean, which means your math is broken. You either have a sign error, an uninitialized buffer, or a mismatch between your forward and backward passes. _Your context window becomes `backward_pass.cu`._

Run that isolation test on a single text file. Let the engine tell you where the bug is.

A perfectly smooth, downward-sloping loss curve only exists in textbooks. In reality, neural networks train on "mini-batches" (small chunks of data at a time), which introduces natural mathematical noise.

When your loss drops from 10 to 8 to 7, and then bumps to 7.5 or 8, that is normal **batch variance**. The engine simply pulled a chunk of data that was slightly harder to predict than the last chunk. However, if it jumps violently (e.g., from 7 to 9 or 15) or **consistently increases step after step**, you are experiencing divergence.

Divergence happens for two reasons, both directly related to the two variables you just changed:

1. **Learning Rate Overshoot (Code):** Your optimizer took a mathematical step that was too large, physically launching the weights out of the local minimum valley and up the other side of the cliff.
2. **Maximum Entropy (Data):** You fed the tokenizer complete garbage (like the raw binary PDF image bytes we discussed earlier), meaning the engine is trying to find patterns in static.

Because you changed the backward pass code _and_ the dataset at the exact same time, you are flying blind. You must isolate the variables immediately.

Here is the gold-standard diagnostic procedure to prove exactly whether your new C++ backward pass is broken, or if your new data is garbage:

### The "Single Batch Overfit" Test

Do not look at the code or the data yet. Run this mechanical test to force the engine to prove its own math.

1. **Isolate a Micro-Dataset:** Create a tiny `.txt` file containing exactly one sentence (e.g., "The quick brown fox jumps over the lazy dog."). Remove all other 70 PDFs from the `asset_sample/` directory.
2. **Disable Shuffle:** Ensure the engine reads this exact same sentence every single step.
3. **Run the Training Loop:** Launch your training mode and watch the telemetry for 100 steps.

**How to read the result:**

- **If the loss drops to ~0.0:** Your backward pass code is mathematically perfect. The engine successfully memorized the sentence. The problem is definitively your **new data** (it is either too noisy, not tokenized correctly, or contains binary static).
- **If the loss increases, explodes, or stays high:** Your **new backward pass code** is broken.

### If the Code is Broken (Where to look)

If the overfit test fails, look directly at the changes you made to your CUDA kernels. An actively increasing loss means the optimizer is doing **gradient ascent** instead of gradient descent.

Check these three specific traps in your `backward_pass.cu`:

- **The Sign Error:** Verify your weight update is `W = W - (learning_rate * gradient)`. A single flipped `+` sign will cause the loss to climb infinitely.
- **Overwriting vs. Accumulating:** If multiple threads or steps write to the same gradient buffer, ensure you are using `atomicAdd()` or `+=`. If you use `=`, you are overwriting the gradients and destroying the math.
- **Uninitialized AdamW Buffers:** If your momentum (`m`) and variance (`v`) tensors in the AdamW optimizer weren't explicitly zeroed out (`cudaMemsetAsync` to 0) before the run, they will contain random VRAM garbage, instantly corrupting your weights on step 1.

Run the Single Batch Overfit test right now. Does the loss drop to zero, or does it explode?
