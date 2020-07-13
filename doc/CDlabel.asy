size(11.7cm,11.7cm);

//settings.tex="pdflatex"; // Work around bug in latest git version of dvisvgm.
asy(nativeformat(),"logo");

fill(unitcircle^^(scale(2/11.7)*unitcircle),
     evenodd+rgb(124/255,205/255,124/255));
label(scale(1.1)*minipage(
"\centering\scriptsize \textbf{\LARGE {\tt Asymptote}\\
\smallskip
\small The Vector Graphics Language}\\
\smallskip
\textsc{Andy Hammerlindl, John Bowman, and Tom Prince}
http://asymptote.sourceforge.net\\
",8cm),(0,0.6));
label(graphic("logo","height=7cm"),(0,-0.22));
clip(unitcircle^^(scale(2/11.7)*unitcircle),evenodd);
