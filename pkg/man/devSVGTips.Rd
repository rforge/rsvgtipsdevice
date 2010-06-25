\name{devSVGTips}
\alias{devSVGTips}
\title{A graphics device for SVG with dynamic tips and hyperlinks}
\usage{
devSVGTips(file = "Rplots.svg", width = 7, height = 7,
       bg = "white", fg = "black", onefile=TRUE,
       xmlHeader=FALSE, useStyleAttributes=FALSE,
       toolTipMode = 1, toolTipFontSize = 10,
       toolTipOpacity = 1.0, title = "R SVG Plot", sub.special=TRUE)
}
\arguments{
  \item{file}{the file where output will appear}
  \item{width}{The width of the plot in inches}
  \item{height}{the height of the plot in inches}
  \item{bg}{the background color for the plot}
  \item{fg}{the foreground color for the plot}
  \item{onefile}{merge plot calls into onefile or separate them to separate pages}
  \item{xmlHeader}{Print XML header or not (it is recommended to not print a header)}
  \item{useStyleAttributes}{Specify shape attributes in a
    \code{style} attribute or as plain attributes (plain attributes is recommended)}
  \item{toolTipMode}{Mode of toop tips: 0 (no tool tips), 1, or 2 (number of lines of tool tip)}
  \item{toolTipFontSize}{Size of font in tool tips (in points/pixels)}
  \item{toolTipOpacity}{Opacity of toop tips: between 0 and 1.  A value
    of 0.9 allows the image behind the tooltip to be seen, but can
    render slowly on some viewers}
  \item{title}{Title of plot}
  \item{sub.special}{Unless \code{sub.special=FALSE}, special SVG
    characters (ampersand, less-than, etc.) in \code{title} will be substituted by the
    appropriate XML encoding, except for an ampersand followed by
    lower-case letters and then a semi-colon (if these substitutions are not made, it is easy to
    inadvertently create unusable SVG files)}
}
\description{
  This function starts up an SVG graphics device that will write to a
  file.  The SVG shapes and text have optional tooltips and/or
  hyperlinks.
}
\details{
  This graphics do not appear in any window while they are being drawn
  -- SVG commands are written to a file and can be viewed in an SVG
  viewer after the plot is completed.

  After the plot is completed, the function \code{\link{dev.off}} must
  be called to close the graphics device and flush all unwritten SVG
  commands to the file.

  Text drawn on the graphic by commands like \code{text()} is subject to
  XML special characters being replaced by the corresponding XML encoding,
  except for an ampersand followed by
  lower-case letters and then a semi-colon.  This behavior is NOT
  controlled by \code{sub.special} and there is currently no way to
  turn it off.

  This device is not implemented as one that can do clipping.  Thus, R
  commands like \code{text} will not draw the text if part of the text
  might be outside the plotting region.  This can be the cause of
  \code{legend} (or other functions that call \code{text}) omitting
  text.  The solution is to make sure that the text is drawn inside the
  plotting region, e.g., using commands like \code{legend(...,
    inset=0.01)}.
  If text is not appearing, use \code{par(xpd=NA)}, which will do the
  least clipping of text.  The example plot svgplot13.svg in
  \link{RSVGTipsDevice} shows clipping that happens for various settings
  of \code{par("xpd")}.

  The clipping behavior can result in labels unexpectedly not appearing
  in lattice graphics.  This can sometime be solved by making the region
  in which labels are drawn larger, e.g., by doing something like
  \code{xyplot(strip.par.text=list(lines=2), ...)}.
}
\seealso{
  \link[=SVG.viewing]{SVG viewing} Ways to view SVG files.
  \link[=RSVGTipsDevice.design]{Design and future} of the RSVGTips device.
  \link[=RSVGTipsDevice]{Overview} of the RSVGTips device (many more examples).
  \code{\link{setSVGShapeToolTip}}, \code{\link{setSVGShapeURL}}, \code{\link{getSVGToolTipMode}}, \code{\link{pictex}}, \code{\link{postscript}}, \code{\link{Devices}}.
}
\examples{
\dontrun{
library("RSVGTipsDevice")
devSVGTips("svgplot1.svg", toolTipMode=1, title="SVG example plot 1: shapes and points, tooltips are title + 1 line")
plot(c(0,10),c(0,10), type="n", xlab="x", ylab="y", main="Example SVG plot with title+ 1 line tips (mode=1)")
setSVGShapeToolTip(title="A rectangle", desc="that is yellow")
rect(1,1,4,6, col='yellow')
setSVGShapeToolTip(title="1st circle with title only")
points(5.5,7.5,cex=20,pch=19,col='red')
setSVGShapeToolTip(title="A triangle", desc="big and green")
polygon(c(3,6,8), c(3,6,3), col='green')
# no tooltips on these points
points(2:8, 8:2, cex=3, pch=19, col='black')
# tooltips on each these points
invisible(sapply(1:7, function(x) {setSVGShapeToolTip(title=paste("point", x)); points(x+1, 8-x, cex=3, pch=1, col='black')}))
setSVGShapeToolTip(title="Text", desc="can have a tool tip too!")
text(x=4, y=9, lab="Poke me!", col="blue")
dev.off()

# Not run in tests because uses the SemiPar package for the fuel.frame data
# A plot of fuel mileage vs weight
library("RSVGTipsDevice")
library("SemiPar")
data(fuel.frame)
fuel.frame <- cbind(fuel.frame,
    US=is.element(substring(fuel.frame$car.name, 1, 5),
                  c("Buick", "Chevr", "Chrys", "Dodge", "Eagle",
                    "Ford ", "Mercu", "Oldsm", "Plymo", "Ponti")))
devSVGTips("mlgvswgt1.svg", height=5, width=7, toolTipMode=1, title="Mileage vs Weight for autos, tooltips are title + 1 line")
plot(fuel.frame$Weight, fuel.frame$Mileage, type="n", xlab="Weight",
ylab="Miles per gallon", main="US cars in blue, imports in yellow")
for (i in seq(len=nrow(fuel.frame))) {
    setSVGShapeToolTip(title=fuel.frame[i,"car.name"],
        desc=paste(fuel.frame[i, "Type"], ", disp=", fuel.frame[i,"Disp."]))
    points(fuel.frame[i,"Weight"], fuel.frame[i,"Mileage"], pch=19,
        cex=2, col=if (fuel.frame[i,"US"]) "blue" else "yellow")
}
dev.off()
}
}
\keyword{device}
