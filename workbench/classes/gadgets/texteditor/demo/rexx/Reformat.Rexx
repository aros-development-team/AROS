/*  Reformat a paragraph */

Options Results

Position SOL
SetBookmark 3
Line 1
GetLine
LineContents = RESULT
MoreLines = 1

Do While ((LineContents ~= '0a'x) & (Left(LineContents,1) ~= ' ') & (MoreLines ~= 0))

	Position SOL
	Backspace
	Text ' '

	GetCursor Line
	CurrentLine = RESULT
	Line 1
	GetCursor Line
	if(RESULT = CurrentLine) Then MoreLines = 0

	GetLine
	LineContents = RESULT

End

GotoBookmark 3
GotoColumn 80
GetCursor Column
ColumnNr = RESULT

Do While (ColumnNr = 80)

	Previous Word
	Backspace
	Text '0a'x
	GotoColumn 80
	GetCursor Column
	ColumnNr = RESULT

End

Cursor Right
