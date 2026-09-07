Formatting/Linting
------------------

We use [ruff](https://docs.astral.sh/ruff/) for formatting and as a
linter, and [mypy](https://mypy-lang.org/) for optional type checking.
These are convenience tools - it isn't considered an error for `ruff
check`/`mypy` to find something, or for a file to not be run through
`ruff format`. We use `make lint`/`make format`/`make mypy` (see
`Makefile`) where useful just to reduce noise and catch the occasional
real problem, not as a hard requirement.
