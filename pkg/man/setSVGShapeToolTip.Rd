\name{setSVGShapeToolTip}
\alias{setSVGShapeToolTip}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{ Set the tool tip for the next graphics shape drawn with the RSVGTips device. }
\description{
  Set the tool tip for the next graphics shape drawn with the RSVGTips device.
}
\usage{
setSVGShapeToolTip(title = NULL, desc = NULL, desc1 = desc, desc2 =
NULL, sub.special=TRUE)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{title}{ A title for the tool tip box. (optional)}
  \item{desc}{ The text of the tool tip. }
  \item{desc1}{ The first line of text of the tool tip. }
  \item{desc2}{ The second line of text of the tool tip (used when
    the tool tip mode is 2.) }
  \item{sub.special}{ If \code{FALSE}, do NOT replace special characters
    by their XML encodings (see details).}
}
\details{
  This function sets tool tip text that will be used by the next
  graphics shape drawn with \code{\link{RSVGTipsDevice}}.

  The RSVGTips device "consumes" the tooltip with the first shape that
  it draws, e.g., using \code{rect}, \code{points}, \code{polygon},
  etc.  If multiple objects are drawn (e.g., by supplying a vector to
  \code{points}), only the first shape drawn will have a tooltip.  Note
  that \code{lines} does NOT use a tooltip.

  Unless \code{sub.special=FALSE}, the following special characters in
  \code{title}, \code{desc}, etc. will
  be substituted by the XML entity as shown:
  \tabular{rl}{
  \code{\&} \tab \&amp; \cr
  \code{\'} \tab \&apos; \cr
  \code{\"} \tab \&quot; \cr
  \code{\<} \tab \&lt; \cr
  \code{\>} \tab \&gt;
}
% Note that it was very hard to get all the quoting right in the above table
% so that it could be processed correctly - merely escaping the special
% characters in the first column was no sufficient - had to enclose them
% in \code{} too.
  The exception to this is that an ampersand followed by lower-case
  letters and then a semi-colon is always left alone, because it already
  looks like an XML entity.
}
\value{
  Returns an invisible NULL.
}
\author{ Tony Plate \email{tplate@acm.org}}
\seealso{ Examples here: \code{\link{RSVGTipsDevice}} }
\examples{
\dontrun{setSVGShapeToolTip(title="A rectangle", desc="that is yellow")}
}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{ device }
