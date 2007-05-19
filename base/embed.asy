usepackage("movie15","3D");
usepackage("hyperref","setpagesize=false");

// See http://www.tug.org/tex-archive/macros/latex/contrib/movie15/README
// for documentation of the options.

settings.outformat="pdf";

// Embed object in pdf file 
string embed(string name, string options="", real width=0, real height=0)
{
  if(options != "") options="["+options+"]{";
  if(width != 0) options += (string) (width*pt)+"pt"; 
  options += "}{";
  if(height != 0) options += (string) (height*pt)+"pt"; 
  return "\includemovie"+options+"}{"+name+"}";
}

string hyperlink(string url, string text)
{
  return "\href{"+url+"}{"+text+"}";
}

string link(string label, string text, string options="")
{
  // Run LaTeX twice to resolve references.
  settings.twice=true;
  if(options != "") options="["+options+"]";
  return "\movieref"+options+"{"+label+"}{"+text+"}";
}
