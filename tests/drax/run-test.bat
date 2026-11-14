@REM export MODE=test

.\bin\drax .\tests\drax\print.dx

.\bin\drax .\tests\drax\number.dx

.\bin\drax .\tests\drax\functions.dx

.\bin\drax .\tests\drax\string.dx

.\bin\drax .\tests\drax\frames.dx

.\bin\drax .\tests\drax\large_frames.dx

.\bin\drax .\tests\drax\os.dx

.\bin\drax .\tests\drax\list.dx

.\bin\drax .\tests\drax\import.dx

.\bin\drax .\tests\drax\math.dx

.\bin\drax .\tests\drax\lambdas.dx

.\bin\drax .\tests\drax\lambdas-nested.dx

.\bin\drax .\tests\drax\pipe.dx

./bin/drax .\tests\drax\TCO.dx

.\bin\drax .\tests\drax\operators.dx

.\bin\drax .\tests\drax\types.dx

.\bin\drax .\tests\drax\closures.dx

.\bin\drax .\tests\drax\recursion.dx

set DRAX_MAX_CALL_DEPTH=1000
.\bin\drax .\tests\drax\tail_calls.dx
set DRAX_MAX_CALL_DEPTH=

.\bin\drax .\tests\drax\edge_cases.dx

@REM  .\bin\drax .\tests\drax\http\http-only-response-text-plain.dx
