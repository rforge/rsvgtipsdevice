\name{RSVGTipsDevice.design}
\alias{RSVGTipsDevice.design}
\title{Design and future of the SVG Graphics Driver with dynamic tips}
\description{
  The design of this driver is that it can be used with ordinary R
  plotting commands.  The extra information needed for tooltips and
  hyperlinks is supplied by a call to a separate function call before
  the call to the plotting function that generates the shape.
  Extra information for only one shape can be supplied this way, so
  shapes that have tooltips or hyperlinks must be drawn one at a time.
}
\section{SVG coding style}{
  It appears that preferred SVG coding styles have changed over time.
  The defaults in this driver are those suggested at
  \url{http://jwatt.org/svg/authoring/}.

  In particular:
  \itemize{
    \item The driver output by default does NOT include a DOCTYPE DTD (it is
    recommended at \url{http://jwatt.org/svg/authoring/} to NOT include a
    DOCTYPE declaration.)
    \item Use shape properties directly rather than specifying them via
    style property, e.g., prefer this:
    \code{<circle fill="red" stroke="blue" ... />} over this:
    \code{<circle style="fill:red; stroke:blue;" ... />}.
    \item Use "Namespace" aware functions, e.g.,
    \code{getElementsByTagNameNS} instead of \code{getElementsByTagName}
    (see also "SVG: Namespace Crash Course" in the references).
  }
}
\section{Limitations}{
  \itemize{
    \item
    This driver currently does not have any font metric information,
    so the use of \code{\link{plotmath}} is not supported.
    \item
    This device will not record tool tips and hyperlinks if
    it is called by \code{dev.copy} - the functions to add tool tips and
    hyperlinks are ignored by other devices.
  }
}
\section{Future development possibilities}{
  \itemize{
    \item Clean up the SVG code that defines the layout of a tool tip
    (\code{SVG_footer()} in \code{src/devSVG.c}) - currently the vertical
    spacing (the 'height' and 'y' attributes) is somewhat ad-hoc (coded
    by trial and error, and may not work for all font sizes).
    \item Add font metric information to support the use of
    \code{\link{plotmath}}.
    \itemize{
      \item Note that SVG uses Unicode characters encoded like \code{&\#x3b1;}.
      The symbols for Unicode are described here:
      \url{http://www.unicode.org/charts/symbols.html}.
      \item SVG allows for definitions of fonts - maybe the right
      approach is to include a symbol font in the doc when plotmath is
      used.
      \item \url{http://www.w3.org/TR/SVG11/fonts.html}
      \item \url{http://www.w3.org/TR/xsl/\#font-model}
      }
    \item Clean up the two-line tooltip SVG code:
    The two-line tooltip mode displays OK in the default SVG renderers in Firefox (under Windows
    2000/XP and Linux) but does not display in the Batik standalone SVG
    viewer.  It probably needs declarations to extend XML elements:
    \url{http://www.w3.org/TR/SVG/extend.html}
    23.5 Adding private elements and attributes to the DTD.
    \item Provide post-processing to use CSS (cascading style sheets) to make SVG files with
    lots of elements more compact.
    \item The design of how tooltip and URL information is passed to the
    device is somewhat ugly -- it would be nicer if the basic graphics
    commands
    allowed additional optional arguments.  However, they don't, and
    the scheme implemented here at least works.  It has the advantage that the
    graphics commands that specify tooltips and URLs will be ignored
    when other devices are used.  One disadvantage is graphics recording
    will not record tooltips and URLs.
  }
}
\references{
  www Consortum
  \emph{W3C Scalable Vector Graphics (SVG)}
  \url{http://www.w3.org/Graphics/SVG/Overview.htm8}
  
  SVG authoring guidelines \url{http://jwatt.org/svg/authoring}

  SVG: Namespaces Crash Course
  \url{http://developer.mozilla.org/en/docs/SVG\_Namespace\_Tips\_(external)}

  "SVG Essentials", J. David Eisenberg, O'Reilly and Associates, 2002.
}
\seealso{
  \link[=RSVGTipsDevice]{Overview} of the RSVGTips device.

  \code{\link{pictex}}, \code{\link{postscript}}, \code{\link{Devices}}.

  \code{Cairo} is another device for R that can produce SVG
  graphics; see \url{http://www.rforge.net/Cairo}.  However, it doesn't appear to support tooltips.
}
\keyword{device}
