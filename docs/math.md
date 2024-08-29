# Math

The `math` module in Drax provides a set of essential mathematical functions for various computations. These functions cover trigonometric, logarithmic, exponential, and other common mathematical operations. This document provides an overview of the available functions and how to use them effectively.

### `cos`
- **Description**: Returns the cosine of an angle in radians.
- **Examples**:
  ```drax
  > Math.cos(0)
  1
  ```

### `cosh`
- **Description**: Returns the hyperbolic cosine of a number.
- **Examples**:
  ```drax
  > Math.cosh(0)
  1.0
  ```

### `acos`
- **Description**: Returns the arc cosine of a number in radians.
- **Examples**:
  ```drax
  > Math.acos(0)
  1.5708
  ```

### `floor`
- **Description**: Returns the largest integer less than or equal to a given number.
- **Examples**:
  ```drax
  > Math.floor(2.5)
  2.0
  ```

### `ceil`
- **Description**: Returns the smallest integer greater than or equal to a given number.
- **Examples**:
  ```drax
  > Math.ceil(2.5)
  3.0
  ```

### `pow`
- **Description**: Returns the result of raising a base to a given exponent.
- **Examples**:
  ```drax
  > Math.pow(2, 5)
  32
  ```

### `tan`
- **Description**: Returns the tangent of an angle in radians.
- **Examples**:
  ```drax
  > Math.tan(1)
  1.55741
  ```

### `tanh`
- **Description**: Returns the hyperbolic tangent of a number.
- **Examples**:
  ```drax
  > Math.tanh(1)
  0.761594
  ```

### `sqrt`
- **Description**: Returns the square root of a number.
- **Examples**:
  ```drax
  > Math.sqrt(4)
  2
  ```

### `atan`
- **Description**: Returns the arc tangent of a number in radians.
- **Examples**:
  ```drax
  > Math.atan(1)
  0.785398 // Equivalent to π/4 in radians
  ```

### `atan2`
- **Description**: Returns the arc tangent of the quotient of its two arguments, with the correct quadrant in radians.
- **Examples**:
  ```drax
  > Math.atan2(1, 1)
  0.785398  // Equivalent to π/4 in radians
  ```

### `exp`
- **Description**: Returns e raised to the power of a given number.
- **Examples**:
  ```drax
  > Math.exp(1)
  2.71828  // The value of e
  ```

### `fabs`
- **Description**: Returns the absolute value of a number.
- **Examples**:
  ```drax
  > Math.fabs(-5.5)
  5.5
  ```

### `frexp`
- **Description**: Decomposes a number into a normalized fraction and an exponent. Returns a list with the fraction and the exponent.
- **Examples**:
  ```drax
  > Math.frexp(8.5)
  [0.53125, 4]
  ```

### `ldexp`
- **Description**: Returns `x * (2^exp)`, effectively combining a fraction and an exponent.
- **Examples**:
  ```drax
  > Math.ldexp(0.5, 3)
  4.0
  ```

### `log`
- **Description**: Returns the natural logarithm (base e) of a number.
- **Examples**:
  ```drax
  > Math.log(2)
  0.693147
  ```

### `log10`
- **Description**: Returns the base-10 logarithm of a number.
- **Examples**:
  ```drax
  > Math.log10(100)
  2
  ```

### `modf`
- **Description**: Decomposes a number into its fractional and integral parts. Returns a list with the fractional part and the integral part.
- **Examples**:
  ```drax
  > Math.modf(5.5)
  [0.5, 5]
  ```

### `sin`
- **Description**: Returns the sine of an angle in radians.
- **Examples**:
  ```drax
  > Math.sin(1)
  0.841471
  ```

### `sinh`
- **Description**: Returns the hyperbolic sine of a number.
- **Examples**:
  ```drax
  > Math.sinh(1)
  1.1752
  ```

### `asin`
- **Description**: Returns the arc sine of a number in radians.
- **Examples**:
  ```drax
  > Math.asin(1)
  1.5708
  ```