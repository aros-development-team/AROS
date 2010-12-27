using System;
using System.Collections.Generic;
using System.IO;


namespace glstubgenerator
{
	class Argument
	{
		public string Type;
		public string Name;
		public string Register;
	}
	
	class ArgumentList: List<Argument>
	{
	}
	
	class Function
	{
		public string ReturnType;
		public string Name;
		public ArgumentList Arguments = new ArgumentList();
		
		public void CalculateRegisters()
		{
			int nextdreg = 0;
			int nextareg = 0;
			
			foreach(Argument a in Arguments)
			{
				if ((a.Type.IndexOf("*") >= 0) || (nextdreg > 7))
				{
					if (nextareg > 5) throw new ApplicationException("A6 reached");
					
					a.Register = string.Format("A{0}", nextareg++);
				}
				else
					a.Register = string.Format("D{0}", nextdreg++);
			}
			
		}
		
		public void CorrectionForArrayArguments()
		{
			bool correctioMade = false;
			foreach(Argument a in Arguments)
			{
				int bracketpos = a.Name.IndexOf("[");
				if (bracketpos >= 0)
				{
					/* Change array to pointer */
					a.Type += " *";
					a.Name = a.Name.Substring(0, bracketpos);
					correctioMade = true;
				}
			}
			
			if (correctioMade)
				Console.WriteLine("Correction for array arguments: {0}", this.Name);
		}
		
		public bool ReturnsVoid()
		{
			if (ReturnType.Equals("void"))
				return true;
			else
				return false;
		}
	}
	
	class FunctionList : List<Function>
	{
		/// <summary>
		/// Removes entries from this list which have the same name as an entry in other list
		/// </summary>
		/// <param name="other">
		/// A <see cref="FunctionList"/>
		/// </param>
		public void RemoveFunctions(FunctionList other)
		{
			/* Copy this list to hashtable */
			Dictionary<string, Function> temp = new Dictionary<string, Function>();
			FunctionList toBeRemoved = new FunctionList();
			
			foreach(Function f in this)
				temp.Add(f.Name, f);
			
			/* Find duplicates */
			foreach(Function f in other)
				if (temp.ContainsKey(f.Name))
				{
					Function thisf = temp[f.Name];
					if (thisf.Arguments.Count != f.Arguments.Count)
						throw new ApplicationException(string.Format("Same name, different arguments count: {0} {1} {2}",
				                               f.Name, f.Arguments.Count, thisf.Arguments.Count));
					toBeRemoved.Add(thisf);
				}
			
			foreach(Function f in toBeRemoved)
				this.Remove(f);
		}
		
		public void RemoveFunctionsExceptFor(FunctionNameDictionary functions)
		{
			FunctionList toBeRemoved = new FunctionList();
			
			foreach(Function f in this)
			{
				if (!functions.ContainsKey(f.Name))
				{
					toBeRemoved.Add(f);
				}
				else
				{
					functions.MarkAsMatched(f.Name);
				}
			}

			foreach(Function f in toBeRemoved)
			{
				this.Remove(f);
			}
		}
		
		public void CorrectionForArrayArguments()
		{
			foreach (Function f in this)
				f.CorrectionForArrayArguments();
		}
		
		public void CalculateRegisters()
		{
			foreach (Function f in this)
				f.CalculateRegisters();
		}
		
		public void ReorderToMatch(FunctionList requestedOrderOfFunctions)
		{
			/* Rather not effective implementation
			 * For each item on ordered list, find it in this list, put to temp list, remove from this list
			 * when finished, copy remaining items on this list to temp list
			 * copy temp list to this list */
			FunctionList tempList = new FunctionList();
			
			foreach(Function ordered in requestedOrderOfFunctions)
			{
				foreach(Function current in this)
				{
					if ((current.Name == ordered.Name) &&
					    current.ReturnType == ordered.ReturnType)
					{
						tempList.Add(current);
						this.Remove(current);
						break;
					}
				}
			}
			
			foreach(Function current in this)
				tempList.Add(current);
			
			this.Clear();
			
			this.AddRange(tempList);
		}
	}

	class FunctionNameDictionary : Dictionary<string, object>
	{
		public void MarkAsMatched(string function)
		{
			if (this.ContainsKey(function))
				this[function] = (object)1;
		}
		
		public void WriteUnmatched()
		{
			foreach (string function in this.Keys)
				if (this[function] == null)
					Console.WriteLine("Unmatched implemented function: {0}", function);
		}
	}
	
	
	
	class GLApiTempParser
	{
		/// <summary>
		/// Path to glapitemp.h file
		/// </summary>
		public FunctionNameDictionary Parse(string path)
		{
			StreamReader sr = File.OpenText(path);
			FunctionNameDictionary functions = new FunctionNameDictionary();
			
			string line = null;
			int matchpos = -1;
			
			while ((line = sr.ReadLine()) != null)
			{
				if ((matchpos = line.IndexOf("KEYWORD2 NAME(")) >= 0)
				{
					int closingbracketpos = line.IndexOf(")", matchpos);
					string fname = "gl" + line.Substring(matchpos + 14, closingbracketpos - 14 - matchpos);
					
					if (fname.IndexOf("_dispatch_stub") >= 0)
						continue; /* Trash not needed */
					
					if (!functions.ContainsKey(fname))
					{
						functions.Add(fname, null);
					}
				}
			}
			
			return functions;
		}
	}
	
	class GLApiTableParser
	{
		/// <summary>
		/// Path to glapioffsets.h file
		/// </summary>
		public FunctionNameDictionary Parse(string path)
		{
			StreamReader sr = File.OpenText(path);
			FunctionNameDictionary functions = new FunctionNameDictionary();
			
			string line = null;
			
			while ((line = sr.ReadLine()) != null)
			{
				if (line.IndexOf("#define _gloffset_") >= 0)
				{
					string part = line.Substring(18, line.Length - 18);
					string fname = "gl" + (part.Split(' ')[0].Trim());
					if (!functions.ContainsKey(fname))
					{
						functions.Add(fname, null);
					}
				}
			}
			
			return functions;
		}
	}
	
	class GLHeaderParser
	{
		public string readandnormalize(StreamReader sr)
		{
			string s = sr.ReadLine();
			
			if (s == null) return null;
			
			s = s.Replace("\n","");
			s = s.Replace("\t","");
			
			return s;
				
		}
		
		public FunctionList Parse(string pathToHeader)
		{
			FunctionList functions = new FunctionList();
			
			StreamReader sr = File.OpenText(pathToHeader);
			
			string line = null;
			int GLAPIposition = -1;
			string GLAPIENTRYstring = "";
			int GLAPIENTRYposition = -1;
			int openbracketposition = -1;
			int closebracketpositiong = -1;

			while((line = readandnormalize(sr)) != null)
			{
				if (line == string.Empty)
					continue;
				
				if (line.IndexOf("#") >= 0)
					continue;
				
				/* check tokens */
				GLAPIposition = line.IndexOf("GLAPI");
				
				if (GLAPIposition < 0)
					continue;
				
				/* Check GLAPIENTRY first */
				GLAPIENTRYstring = "GLAPIENTRY";
				GLAPIENTRYposition = line.IndexOf(GLAPIENTRYstring, GLAPIposition);
				
				if (GLAPIENTRYposition < 0)
				{
					/* Try with APIENTRY */
					GLAPIENTRYstring = "APIENTRY";
					GLAPIENTRYposition = line.IndexOf(GLAPIENTRYstring, GLAPIposition);
					if (GLAPIENTRYposition < 0)
						continue;
				}
					
				
				openbracketposition = line.IndexOf("(", GLAPIENTRYposition);
				
				if (openbracketposition < 0)
					continue;
				
				closebracketpositiong = line.IndexOf(")", openbracketposition);
				
				if (closebracketpositiong < 0)
				{
					/* read next lines for closing brackets */
					string nextline = null;
					
					while((nextline = readandnormalize(sr))!= null)
					{
						line += nextline;
						closebracketpositiong = line.IndexOf(")", openbracketposition);
						if (closebracketpositiong >= 0)
							break;
					}
				}
				
				/* create objects */
				Function f = new Function();
				f.ReturnType = line.Substring(GLAPIposition + 5, GLAPIENTRYposition - GLAPIposition - 5).Trim();
				f.Name = line.Substring(GLAPIENTRYposition + GLAPIENTRYstring.Length, openbracketposition - GLAPIENTRYposition - GLAPIENTRYstring.Length).Trim();
				
				string argumentsstring = line.Substring(openbracketposition + 1, closebracketpositiong - 1 - openbracketposition);

				string [] arguments = argumentsstring.Split(',');

				char nextargumentname = 'a';
				
				foreach (string argument in arguments)
				{
					/* change * and & so that they are no concatenated with variable name */
					string innerargument = argument.Replace("*", " * ");
					innerargument = innerargument.Replace("&", " & ");
					innerargument = innerargument.Replace("  ", " ");
					innerargument = innerargument.Trim();
					
					/* Possible situations:
					 * (A) innerargument = "void"
					 * (B) innerargument = "type variable"
					 * (C) innerargument = "type * variable"
					 * (D) innerargument = "type & variable"
					 * (E) innerargumetn = "type"
					 * (F) innerargument = "type *"
					 * (G) innerargument = "type &"
					 */
					
					string [] argumentparts = innerargument.Split(' ');
					
					/* Detection for A: only one argument with one argumentpart containing void*/
					if ((argumentparts.Length == 1) && (arguments.Length == 1) && (argumentparts[0].IndexOf("void") >= 0))
						continue;
					
					int lastPositionOfTypeBackwards = 1; /* Means the last element of argumentparts is variable name */
					
					/* Detection for E, F, G: argument without variable name */
					if ((argumentparts[argumentparts.Length - 1] == "*") ||
					    (argumentparts[argumentparts.Length - 1] == "&") ||
					    (argumentparts.Length == 1)
						)
					{
						lastPositionOfTypeBackwards = 0; /* Means the last element of argumentparts is type */
					}
					
					Argument arg = new Argument();
					arg.Type = "";
					for (int i = 0; i < argumentparts.Length - lastPositionOfTypeBackwards; i++)
						arg.Type = arg.Type + argumentparts[i] + " ";
					arg.Type = arg.Type.Trim();
					
					if (lastPositionOfTypeBackwards == 1)
						arg.Name = argumentparts[argumentparts.Length - 1].Trim();
					else
					{
						/* Autoname for the variable */
						arg.Name = string.Format("{0}", nextargumentname++);
					}
					
					f.Arguments.Add(arg);
				}
				
				functions.Add(f);
				
				
				/*Console.Write("{0} {1} (", f.ReturnType, f.Name);
				int j;
				if (f.Arguments.Count > 0)
				{
					for (j = 0; j < f.Arguments.Count - 1; j++)
						Console.Write("{0} {1} ({2}), ", f.Arguments[j].Type, f.Arguments[j].Name, f.Arguments[j].Register);
					Console.Write("{0} {1} ({2})", f.Arguments[j].Type, f.Arguments[j].Name, f.Arguments[j].Register);
				}
				Console.WriteLine(");");*/
			}
			
			sr.Close();
			
			
			return functions;
		}
	}
	
	class ConfParser
	{
		public FunctionList Parse(string pathToFile)
		{
			FunctionList functions = new FunctionList();
			
			/* This file might not yet exist */
			if (!File.Exists(pathToFile))
				return functions;
			
			StreamReader sr = File.OpenText(pathToFile);
			
			string line = null;
			int spacePosition = -1;
			int bracketPosition = -1;
			
			while((line = sr.ReadLine()) != null)
			{
				if (line.IndexOf(" gl") < 0)
					continue;

				bracketPosition = line.IndexOf("(", 0);
				if (bracketPosition < 0)
					continue;
				
				spacePosition = line.LastIndexOf(" ", bracketPosition);
				if (spacePosition < 0)
					continue;
				
				Function f = new Function();
				f.ReturnType = line.Substring(0, spacePosition).Trim();
				f.Name = line.Substring(spacePosition + 1, bracketPosition - spacePosition - 1).Trim();
				
				functions.Add(f);
			}
			
			return functions;
		}
	}
	abstract class ArosFileWriter
	{
		public abstract void Write(string path, FunctionList functions);
	}
	
	class StubsFileWriter : ArosFileWriter
	{
		public override void Write (string path, FunctionList functions)
		{
			StreamWriter swStubs = new StreamWriter(path, false);
			
			foreach (Function f in functions)
			{
				swStubs.WriteLine("AROS_LH{0}({1}, {2},", f.Arguments.Count, f.ReturnType, f.Name);
				foreach (Argument a in f.Arguments)
				{
					swStubs.WriteLine("    AROS_LHA({0}, {1}, {2}),", a.Type, a.Name, a.Register);
				}
				swStubs.WriteLine("    struct Library *, MesaBase, 0, Mesa)");
				swStubs.WriteLine("{");
				swStubs.WriteLine("    AROS_LIBFUNC_INIT");
				swStubs.WriteLine();
				swStubs.WriteLine("    SAVE_REG");
				swStubs.WriteLine();
				swStubs.WriteLine("    PUT_MESABASE_IN_REG");
				swStubs.WriteLine();
				if (!f.ReturnsVoid())
				{
					swStubs.Write("    {0} _return = m{1}(", f.ReturnType, f.Name);
				}
				else
				{
					
					swStubs.Write("    m{0}(", f.Name);
				}
				if (f.Arguments.Count > 0)
				{
					int i = 0;
					for (i = 0; i < f.Arguments.Count - 1; i++)
						swStubs.Write("{0}, ", f.Arguments[i].Name);
					swStubs.Write("{0}", f.Arguments[i].Name);
				}
				swStubs.WriteLine(");");
				swStubs.WriteLine();
				swStubs.WriteLine("    RESTORE_REG");
				swStubs.WriteLine();
				if (!f.ReturnsVoid())
				{
					swStubs.WriteLine("    return _return;");
					swStubs.WriteLine();
				}
				swStubs.WriteLine("    AROS_LIBFUNC_EXIT");
				swStubs.WriteLine("}");
				swStubs.WriteLine();
			}
			
			swStubs.Close();
			
		}
	}
	
	class ConfFileWriter : ArosFileWriter
	{
		public override void Write (string path, FunctionList functions)
		{
			StreamWriter swConf = new StreamWriter(path, false);
			
			foreach (Function f in functions)
			{
				swConf.Write("{0} {1}(", f.ReturnType, f.Name);
				if (f.Arguments.Count > 0)
				{
					int i = 0;
					for (i = 0; i < f.Arguments.Count - 1; i++)
						swConf.Write("{0} {1}, ", f.Arguments[i].Type, f.Arguments[i].Name);
					swConf.Write("{0} {1}", f.Arguments[i].Type, f.Arguments[i].Name);
				}
				swConf.Write(") (");

				if (f.Arguments.Count > 0)
				{
					int i = 0;
					for (i = 0; i < f.Arguments.Count - 1; i++)
						swConf.Write("{0}, ", f.Arguments[i].Register);
					swConf.Write("{0}", f.Arguments[i].Register);
				}
				
				swConf.WriteLine(")");

			}
			
			swConf.Close();		
		}

	}
	
	class UndefFileWriter : ArosFileWriter
	{
		public override void Write (string path, FunctionList functions)
		{
			StreamWriter swUndef = new StreamWriter(path, false);
				
			foreach (Function f in functions)
			{
				swUndef.WriteLine("#undef {0}", f.Name);
			}
			
			swUndef.Close();
		}
	}
	
	class MainClass
	{

		public static void Main(string[] args)
		{
			string PATH_TO_MESA = @"/data/deadwood/AROS/AROS/workbench/libs/mesa/";
			GLApiTempParser apiParser = new GLApiTempParser();
			FunctionNameDictionary implementedFunctions = 
				apiParser.Parse(PATH_TO_MESA + @"/src/mapi/glapi/glapitemp.h");
			
			
			Console.WriteLine("Implemented functions: {0}", implementedFunctions.Keys.Count);
			
			GLHeaderParser p = new GLHeaderParser();
			
			FunctionList functionsglh = p.Parse(PATH_TO_MESA + @"/include/GL/gl.h");
			
			FunctionList functionsglexth = p.Parse(PATH_TO_MESA + @"/include/GL/glext.h");
			
			ConfParser confParser = new ConfParser();
			FunctionList orderedExistingFunctions = confParser.Parse(PATH_TO_MESA + @"/src/mesa/arosmesa.conf");
			
			Console.WriteLine("Initial parse results: GL: {0} GLEXT: {1}", functionsglh.Count, functionsglexth.Count);
			
			functionsglexth.RemoveFunctionsExceptFor(implementedFunctions);
			functionsglh.RemoveFunctionsExceptFor(implementedFunctions);
			
			implementedFunctions.WriteUnmatched();
			
			Console.WriteLine("After filtering of unimplemented functions: GL: {0} GLEXT: {1}", functionsglh.Count, functionsglexth.Count);
			
			functionsglexth.RemoveFunctions(functionsglh);
			Console.WriteLine("After duplicates removal GL: {0}, GLEXT: {1}", functionsglh.Count, functionsglexth.Count);
			functionsglh.AddRange(functionsglexth);
			functionsglexth.Clear();
			Console.WriteLine("After merging GL: {0}, GLEXT: {1}", functionsglh.Count, functionsglexth.Count);

			functionsglh.CorrectionForArrayArguments();
			functionsglh.CalculateRegisters();
			functionsglh.ReorderToMatch(orderedExistingFunctions);
			
			
			StubsFileWriter sfw = new StubsFileWriter();
			sfw.Write(@"/data/deadwood/temp/aros_libapi.c", functionsglh);
			
			ConfFileWriter cfw = new ConfFileWriter();
			cfw.Write(@"/data/deadwood/temp/arosmesa.conf", functionsglh);
			
			UndefFileWriter ufw = new UndefFileWriter();
			ufw.Write(@"/data/deadwood/temp/mangle_undef.h", functionsglh);
			
			
		}
	}
}