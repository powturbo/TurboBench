@rem Usage: bench file [options]
@rem Please note that modes 9..10 require 4gb RAM and 64-bit Windows
@rem
@echo %*
@copy /b %1 nul >nul
@for /L %%a in (1 1 16) do @tor -%%a -o -qh -cpu %*
:: >>d:\res