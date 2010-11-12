\name{SVG.viewing}
\alias{SVG.viewing}
\docType{package}
\title{Ways to view SVG files}
\description{

  The easiest way to view SVG files is in a web browser that has native
  SVG support.  Mozilla Firefox, Opera, Safari, Google Chrome, and
  Microsoft Internet Explorer (version 9 onwards) all have native SVG
  browser support.

  The section "Necessary Viewer/Browser for the examples" at the bottom
  of \url{http://www.carto.net/papers/svg/samples/} has information
  about SVG viewing software for various OS's.
}
\section{Opera notes}{
  The Opera viewer has automatic tooltips, which get data from the
  \code{"title"} element.  When viewing SVG files created by
  RSVGTipsDevice, an extra tooltip will pop up after a short wait.  To
  enable or disable tooltip popups in Opera, go to  Tools > Preferences
  > Advanced > Browsing  and toggle the "Show tooltips" option. (see \url{http://www.opera.com/support/search/view/714/})
}
\keyword{device}
\keyword{package}
