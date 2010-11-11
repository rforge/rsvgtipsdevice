\name{getSVGToolTipMode}
\alias{getSVGToolTipMode}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{Return the tool tip mode of the RSVGTips device}
\description{
  Return the tool tip mode of the RSVGTips device.
}
\usage{
getSVGToolTipMode()
}
%- maybe also 'usage' for other objects documented here.
\details{
  Returns the mode that was set in the call to \code{\link{devSVGTips}}
  (this mode cannot be changed while a plot is being created.)
}
\value{
  An integer representing the tooltip mode of the current device:
  \itemize{
    \item{-1}{: the current device is not an RSVGTips device}
    \item{0}{: the tooltips are not being used}
    \item{1}{: tool tips are title + one line of description}
    \item{2}{: tool tips are title + two lines of description}
  }
}
\author{ Tony Plate \email{tplate@acm.org}}
\seealso{ \code{\link{devSVGTips}} }
\examples{
}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{ device }

