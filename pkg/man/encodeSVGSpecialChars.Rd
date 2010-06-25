\name{encodeSVGSpecialChars}
\alias{encodeSVGSpecialChars}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{ Encode SVG special chars (ampersand, etc) }
\description{
  Encode SVG special chars (ampersand, etc) as the appropriate XML encoding.
}
\usage{
encodeSVGSpecialChars(x, sub.special = TRUE, xent = FALSE)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{x}{ A vector of character data to be encode. }
  \item{sub.special}{ If \code{FALSE}, do not encode. }
  \item{xent}{ If \code{FALSE}, encode as XML encoding, if \code{TRUE},
    encode as XML entity}
}
\details{
  The following substitutions are made:
  \tabular{rll}{
    Character \tab XML encoding \tab XML entity \cr
  \code{\&} \tab \&amp; \tab \&\#38 \cr
  \code{\'} \tab \&apos; \tab \&\#30 \cr
  \code{\"} \tab \&quot; \tab \&\#34 \cr
  \code{\<} \tab \&lt; \tab \&\#60 \cr
  \code{\>} \tab \&gt; \tab \&\#62 
  }

}
\value{
  The vector \code{x} with encodings as appropriate.
}
\author{ Tony Plate \email{tplate@acm.org}}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{ device }
