% Copyright © 1995-2001, The AROS Development Team. 
% $Id$

\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{aros}[1997/06/20 AROS Documentation Commands]

% Process our options... we don't have any yet :-(

% Send all unknown options off to the parent class.
\DeclareOption*{ %
\PassOptionsToClass{\CurrentOption}{book} %
\PassOptionsToClass{\CurrentOption}{report} %
}
\ProcessOptions\relax

\LoadClass{book}

% Unfortunately, we have to make the page slightly larger.
% However, there is still some room left...
\textwidth=15cm \oddsidemargin=5mm \evensidemargin=5mm

\RequirePackage{shortvrb}
\RequirePackage{verbatim}
\RequirePackage{indentfirst}
\AtBeginDocument{\MakeShortVerb{\|}}

% This is where we define the commands that .src files have, 
% but LaTeX doesn't have.

% Normal inline text commands.
\newcommand{\italics}[1]{\textit{#1}}
\newcommand{\bold}[2]{\textbf{#1}}
\newcommand{\shell}[1]{\texttt{#1}}
\newcommand{\filename}[1]{#1}
\newcommand{\email}[1]{\textsl{#1}}
\newcommand{\link}[2]{#1 (\texttt{#2})}
\newcommand{\exec}[1]{\relax}
\newcommand{\execverb}[1]{\relax}
\newcommand{\largepic}[2]{
    \begin{figure} %
      \center{Please insert #2} %
      \caption{#1} %
    \end{figure} %
}

% An environment for examples. Based on the verbatim package.
\newenvironment{example}{\verbatim}{\endverbatim}
