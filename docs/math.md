# Math

The `math` module in Drax provides a set of essential mathematical functions for various computations. These functions cover trigonometric, logarithmic, exponential, and other common mathematical operations. This document provides an overview of the available functions and how to use them effectively.

### `cos`
- **Description**: Returns the cosine of an angle in radians.
- **Examples**:
  ```drax
  > math.cos(0)
  1
  ```

### `cosh`
- **Description**: Returns the hyperbolic cosine of a number.
- **Examples**:
  ```drax
  > math.cosh(0)
  1.0
  ```

### `acos`
- **Description**: Returns the arc cosine of a number in radians.
- **Examples**:
  ```drax
  > math.acos(0)
  1.5708
  ```

### `floor`
- **Description**: Returns the largest integer less than or equal to a given number.
- **Examples**:
  ```drax
  > math.floor(2.5)
  2.0
  ```

### `ceil`
- **Description**: Returns the smallest integer greater than or equal to a given number.
- **Examples**:
  ```drax
  > math.ceil(2.5)
  3.0
  ```

### `pow`
- **Description**: Returns the result of raising a base to a given exponent.
- **Examples**:
  ```drax
  > math.pow(2, 5)
  32
  ```

### `tan`
- **Description**: Returns the tangent of an angle in radians.
- **Examples**:
  ```drax
  > math.tan(1)
  1.55741
  ```

### `tanh`
- **Description**: Returns the hyperbolic tangent of a number.
- **Examples**:
  ```drax
  > math.tanh(1)
  0.761594
  ```

### `sqrt`
- **Description**: Returns the square root of a number.
- **Examples**:
  ```drax
  > math.sqrt(4)
  2
  ```

### `atan`
- **Description**: Returns the arc tangent of a number in radians.
- **Examples**:
  ```drax
  > math.atan(1)
  0.785398 // Equivalent to π/4 in radians
  ```

### `atan2`
- **Description**: Returns the arc tangent of the quotient of its two arguments, with the correct quadrant in radians.
- **Examples**:
  ```drax
  > math.atan2(1, 1)
  0.785398  // Equivalent to π/4 in radians
  ```

### `exp`
- **Description**: Returns e raised to the power of a given number.
- **Examples**:
  ```drax
  > math.exp(1)
  2.71828  // The value of e
  ```

### `fabs`
- **Description**: Returns the absolute value of a number.
- **Examples**:
  ```drax
  > math.fabs(-5.5)
  5.5
  ```

### `frexp`
- **Description**: Decomposes a number into a normalized fraction and an exponent. Returns a list with the fraction and the exponent.
- **Examples**:
  ```drax
  > math.frexp(8.5)
  [0.53125, 4]
  ```

### `ldexp`
- **Description**: Returns `x * (2^exp)`, effectively combining a fraction and an exponent.
- **Examples**:
  ```drax
  > math.ldexp(0.5, 3)
  4.0
  ```

### `log`
- **Description**: Returns the natural logarithm (base e) of a number.
- **Examples**:
  ```drax
  > math.log(2)
  0.693147
  ```

### `log10`
- **Description**: Returns the base-10 logarithm of a number.
- **Examples**:
  ```drax
  > math.log10(100)
  2
  ```

### `modf`
- **Description**: Decomposes a number into its fractional and integral parts. Returns a list with the fractional part and the integral part.
- **Examples**:
  ```drax
  > math.modf(5.5)
  [0.5, 5]
  ```

### `sin`
- **Description**: Returns the sine of an angle in radians.
- **Examples**:
  ```drax
  > math.sin(1)
  0.841471
  ```

### `sinh`
- **Description**: Returns the hyperbolic sine of a number.
- **Examples**:
  ```drax
  > math.sinh(1)
  1.1752
  ```

### `asin`
- **Description**: Returns the arc sine of a number in radians.
- **Examples**:
  ```drax
  > math.asin(1)
  1.5708
  ```