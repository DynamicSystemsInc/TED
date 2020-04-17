### Snap-Now Use in Phoenix Trusted Desktop

`snap-now` is a kludge because the files logically belong in
pkg://solaris/desktop/time-slider, but this component
is already delivered by Oracle. The files in `snap-now`
are only used by `caja`, which isn’t part of Oracle Solaris.
They previously were used by `nautilus` (which became `caja`),
but the version of `nautilus` in Solaris 11.4 doesn’t support
`time-slider` anymore.

I could have added the files to `components/desktop/time-slider`
but I didn’t want to override that with a Phoenix derivative.
I could have added the files to `caja`, but that looked hard at the time.
