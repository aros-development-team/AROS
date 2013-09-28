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
					if (nextareg > 5) 
						throw new ApplicationException("A6 reached");
					
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
		
		public void RemoveFunctionByName(string functionName)
		{
			FunctionList toBeRemoved = new FunctionList();
			
			foreach(Function f in this)
			{
				if (f.Name == functionName)
					toBeRemoved.Add(f);
			}

			foreach(Function f in toBeRemoved)
			{
				this.Remove(f);
			}			
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
	
	class APIHeaderParser
	{
		public const string APIENTRY = "APIENTRY";
		public const string GLAPI = "GLAPI";
		public const string GLAPIENTRY = "GLAPIENTRY";
		public const string EGLAPI = "EGLAPI";
		public const string EGLAPIENTRY = "EGLAPIENTRY";
		public const string VGAPI = "VG_API_CALL";
		public const string VGAPIENTRY = "VG_API_ENTRY";
		public const string VGUAPI = "VGU_API_CALL";
		public const string VGUAPIENTRY = "VGU_API_ENTRY";
		
		public string readandnormalize(StreamReader sr)
		{
			string s = sr.ReadLine();
			
			if (s == null) return null;
			
			s = s.Replace("\n","");
			s = s.Replace("\t","");
			
			return s;
				
		}
		
		public FunctionList Parse(string pathToHeader, string APIstring, string APIENTRYstring)
		{
			FunctionList functions = new FunctionList();
			
			StreamReader sr = File.OpenText(pathToHeader);
			
			string line = null;
			int APIposition = -1;
			int APIENTRYposition = -1;
			int openbracketposition = -1;
			int closebracketpositiong = -1;

			while((line = readandnormalize(sr)) != null)
			{
				if (line == string.Empty)
					continue;
				
				if (line.IndexOf("#") >= 0)
					continue;
				
				/* check tokens */
				APIposition = line.IndexOf(APIstring);
				
				if (APIposition < 0)
					continue;
				
				/* Check APIENTRY first */
				APIENTRYposition = line.IndexOf(APIENTRYstring, APIposition);
				
				if (APIENTRYposition < 0)
					continue;
				if (line[APIENTRYposition - 1] != ' ') /* Space before APIENTRY is required */
					continue;
				
				openbracketposition = line.IndexOf("(", APIENTRYposition);
				
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
				f.ReturnType = line.Substring(APIposition + APIstring.Length, APIENTRYposition - APIposition - APIstring.Length).Trim();
				f.Name = line.Substring(APIENTRYposition + APIENTRYstring.Length, openbracketposition - APIENTRYposition - APIENTRYstring.Length).Trim();
				
				string argumentsstring = line.Substring(openbracketposition + 1, closebracketpositiong - 1 - openbracketposition);

				string [] arguments = argumentsstring.Split(',');

				char nextargumentname = 'a';
				
				foreach (string argument in arguments)
				{
					/* change * and & so that they are no concatenated with variable name */
					string innerargument = argument.Replace("*", " * ");
					innerargument = innerargument.Replace("&", " & ");
					innerargument = innerargument.Replace("  ", " ");
					innerargument = innerargument.Replace(" [", "[");
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
				if ((line.IndexOf(" gl") < 0) && 
				    (line.IndexOf(" egl") < 0) && 
				    (line.IndexOf(" vg") < 0))
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
		protected string getDefine(string path)
		{
			string define = Path.GetFileName(path);
			define = define.Replace('.', '_');
			define = define.ToUpper();
			return define;
		}

		public abstract void Write(string path, FunctionList functions);
	}
	
	class StubsFileWriter : ArosFileWriter
	{
		private bool addRegSaveRest;
		private string baseName;
		private string functionPrefix;
		private int firstFunctionLVO;
		
		public StubsFileWriter(bool addRegSaveRest, string libraryName, int firstFunctionLVO)
		{
			this.addRegSaveRest = addRegSaveRest;
			this.baseName = libraryName + "Base";
			this.functionPrefix = libraryName.ToLower();
			this.functionPrefix = char.ToUpper(this.functionPrefix[0]) + this.functionPrefix.Substring(1);
			this.firstFunctionLVO = firstFunctionLVO;
			
		}

		public override void Write (string path, FunctionList functions)
		{
			StreamWriter swStubs = new StreamWriter(path, false);
			int lvo = firstFunctionLVO;
			
			foreach (Function f in functions)
			{
				swStubs.WriteLine("AROS_LH{0}({1}, {2},", f.Arguments.Count, f.ReturnType, f.Name);
				foreach (Argument a in f.Arguments)
				{
					swStubs.WriteLine("    AROS_LHA({0}, {1}, {2}),", a.Type, a.Name, a.Register);
				}
				swStubs.WriteLine("    struct Library *, {0}, {1}, {2})", baseName, lvo++ ,functionPrefix);
				swStubs.WriteLine("{");
				swStubs.WriteLine("    AROS_LIBFUNC_INIT");
				swStubs.WriteLine();
				if (addRegSaveRest)
				{
					swStubs.WriteLine("    SAVE_REG");
					swStubs.WriteLine();
					swStubs.WriteLine("    PUT_MESABASE_IN_REG");
					swStubs.WriteLine();
				}
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
				if (addRegSaveRest)
				{
					swStubs.WriteLine("    RESTORE_REG");
					swStubs.WriteLine();
				}
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
	
	class MangleFileWriter : ArosFileWriter
	{
		public override void Write (string path, FunctionList functions)
		{
			StreamWriter swMangle = new StreamWriter(path, false);
			
			string define = getDefine(path);

			swMangle.WriteLine("#ifndef {0}", define);
			swMangle.WriteLine("#define {0}", define);
			
			foreach (Function f in functions)
			{
				swMangle.WriteLine("#define {0} m{0}", f.Name);
			}
			
			swMangle.WriteLine("#endif");
			
			swMangle.Close();
		}
	}	

	class MangledHeaderFileWriter : ArosFileWriter
	{
		public override void Write (string path, FunctionList functions)
		{
			StreamWriter swMangledHeader = new StreamWriter(path, false);

			string define = getDefine(path);
			
			swMangledHeader.WriteLine("#ifndef {0}", define);
			swMangledHeader.WriteLine("#define {0}", define);
			
			foreach (Function f in functions)
			{
				swMangledHeader.Write("{0} m{1} (", f.ReturnType, f.Name);
				if (f.Arguments.Count > 0)
				{
					int i = 0;
					for (i = 0; i < f.Arguments.Count - 1; i++)
						swMangledHeader.Write("{0} {1}, ", f.Arguments[i].Type, f.Arguments[i].Name);
					swMangledHeader.Write("{0} {1}", f.Arguments[i].Type, f.Arguments[i].Name);
				}
				swMangledHeader.WriteLine(");");
			}
			
			swMangledHeader.WriteLine("#endif");
			
			swMangledHeader.Close();
		}
	}
	
	class MangledImplementationFileWriter : ArosFileWriter
	{
		public override void Write (string path, FunctionList functions)
		{
			StreamWriter swMangledImplementation = new StreamWriter(path, false);

			foreach (Function f in functions)
			{
				swMangledImplementation.Write("{0} m{1} (", f.ReturnType, f.Name);
				if (f.Arguments.Count > 0)
				{
					int i = 0;
					for (i = 0; i < f.Arguments.Count - 1; i++)
						swMangledImplementation.Write("{0} {1}, ", f.Arguments[i].Type, f.Arguments[i].Name);
					swMangledImplementation.Write("{0} {1}", f.Arguments[i].Type, f.Arguments[i].Name);
				}
				swMangledImplementation.WriteLine(")");
				swMangledImplementation.WriteLine("{");
				if (!f.ReturnsVoid())
					swMangledImplementation.WriteLine("    {0} _ret;", f.ReturnType);

				if (f.Name.Equals("glEnd"))
					swMangledImplementation.WriteLine("    /* glBegin/glEnd must be atomic */");
				else
					swMangledImplementation.WriteLine("    HOSTGL_PRE");
				swMangledImplementation.WriteLine("    D(bug(\"TASK: 0x%x, {0}\", FindTask(NULL)));", f.Name);

				if (f.ReturnsVoid())
					swMangledImplementation.Write("    GLCALL({0}", f.Name);
				else
					swMangledImplementation.Write("    _ret = GLCALL({0}", f.Name);
				if (f.Arguments.Count > 0)
				{
					int i = 0;
					for (i = 0; i < f.Arguments.Count; i++)
						swMangledImplementation.Write(", {0}", f.Arguments[i].Name);
				}
				swMangledImplementation.WriteLine(");");

				swMangledImplementation.WriteLine("    D(bug(\"...exit\\n\"));");
				if (f.Name.Equals("glBegin"))
					swMangledImplementation.WriteLine("    /* glBegin/glEnd must be atomic */");
				else
					swMangledImplementation.WriteLine("    HOSTGL_POST");

				if (!f.ReturnsVoid())
					swMangledImplementation.WriteLine("    return _ret;");
				swMangledImplementation.WriteLine("}");
				swMangledImplementation.WriteLine();
			}

			swMangledImplementation.Close();
		}
	}
	
	class GLFUNCFileWriter : ArosFileWriter
	{
		public override void Write (string path, FunctionList functions)
		{
			StreamWriter swGLFUNC = new StreamWriter(path, false);

			swGLFUNC.WriteLine("struct gl_func {");

			foreach (Function f in functions)
			{
				swGLFUNC.Write("    {0} (*{1}) (", f.ReturnType, f.Name);
				if (f.Arguments.Count > 0)
				{
					int i = 0;
					for (i = 0; i < f.Arguments.Count - 1; i++)
						swGLFUNC.Write("{0} {1}, ", f.Arguments[i].Type, f.Arguments[i].Name);
					swGLFUNC.Write("{0} {1}", f.Arguments[i].Type, f.Arguments[i].Name);
				}
				swGLFUNC.WriteLine(");");
			}
			
			swGLFUNC.WriteLine("};");
			
			swGLFUNC.WriteLine();swGLFUNC.WriteLine();swGLFUNC.WriteLine();swGLFUNC.WriteLine();
			
			swGLFUNC.WriteLine("static const char *gl_func_names[] = {");
			foreach (Function f in functions)
			{
				swGLFUNC.WriteLine("    \"{0}\",", f.Name);
			}
			swGLFUNC.WriteLine("    NULL");
			swGLFUNC.WriteLine("};");
			
			swGLFUNC.Close();
		}
	}
	
	class MainClass
	{
		public static void Main(string[] args)
		{
			string PATH_TO_MESA = @"/data/deadwood/gitV0AROS/AROS/workbench/libs/mesa/";
			GLApiTempParser apiParser = new GLApiTempParser();
			FunctionNameDictionary implementedFunctions = 
				apiParser.Parse(PATH_TO_MESA + @"/src/mapi/glapi/glapitemp.h");
			
			
			Console.WriteLine("Implemented functions: {0}", implementedFunctions.Keys.Count);
			
			/* Parsing part */
			APIHeaderParser p = new APIHeaderParser();
			
			FunctionList functionsglh = p.Parse(PATH_TO_MESA + @"/include/GL/gl.h", APIHeaderParser.GLAPI, APIHeaderParser.GLAPIENTRY);
			FunctionList functionsglhquirk = p.Parse(PATH_TO_MESA + @"/include/GL/gl.h", APIHeaderParser.GLAPI, APIHeaderParser.APIENTRY);
			functionsglh.AddRange(functionsglhquirk);
			
			FunctionList functionsglexth = p.Parse(PATH_TO_MESA + @"/include/GL/glext.h", APIHeaderParser.GLAPI, APIHeaderParser.APIENTRY);
			

			ConfParser confParser = new ConfParser();
			FunctionList orderedExistingFunctions = confParser.Parse(PATH_TO_MESA + @"/src/aros/arosmesa/arosmesa.conf");
			
			Console.WriteLine("Initial parse results: GL: {0} GLEXT: {1}", functionsglh.Count, functionsglexth.Count);
			
			functionsglexth.RemoveFunctionsExceptFor(implementedFunctions);
			functionsglh.RemoveFunctionsExceptFor(implementedFunctions);
			
			implementedFunctions.WriteUnmatched();
			
			Console.WriteLine("After filtering of unimplemented functions: GL: {0} GLEXT: {1}", functionsglh.Count, functionsglexth.Count);
			
			/* Generation part */
			
			/* GL */
			FunctionList functionsGL = new FunctionList();
			
			functionsglexth.RemoveFunctions(functionsglh);
			
			Console.WriteLine("After duplicates removal GL: {0}, GLEXT: {1}", functionsglh.Count, functionsglexth.Count);
			functionsGL.AddRange(functionsglh);
			functionsGL.AddRange(functionsglexth);

			Console.WriteLine("After merging GL {0}", functionsGL.Count);

			
			FunctionList functionsfinal = new FunctionList();
			functionsfinal.AddRange(functionsGL);
			
			functionsfinal.CorrectionForArrayArguments();
			functionsfinal.CalculateRegisters();
			functionsfinal.ReorderToMatch(orderedExistingFunctions);
			
			
			StubsFileWriter sfw = new StubsFileWriter(false, "Mesa", 35);
			sfw.Write(@"/data/deadwood/temp/arosmesa_library_api.c", functionsfinal);
			
			ConfFileWriter cfw = new ConfFileWriter();
			cfw.Write(@"/data/deadwood/temp/arosmesa.conf", functionsfinal);
			
			MangleFileWriter glmfw = new MangleFileWriter();
			glmfw.Write(@"/data/deadwood/temp/arosmesa_mangle.h", functionsfinal);

			MangledHeaderFileWriter glmhfw = new MangledHeaderFileWriter();
			glmhfw.Write(@"/data/deadwood/temp/arosmesaapim.h", functionsfinal);
			
			MangledImplementationFileWriter glmifw = new MangledImplementationFileWriter();
			glmifw.Write(@"/data/deadwood/temp/hostgl_gl_api.c", functionsfinal);

			GLFUNCFileWriter glfuncfw = new GLFUNCFileWriter();
			glfuncfw.Write(@"/data/deadwood/temp/gl_func.ch", functionsfinal);
			
			/* EGL */
			FunctionList functionseglh = p.Parse(PATH_TO_MESA + @"/include/EGL/egl.h", APIHeaderParser.EGLAPI, APIHeaderParser.EGLAPIENTRY);
			FunctionList orderedExistingFunctionsEGL = confParser.Parse(PATH_TO_MESA + @"/src/aros/egl/egl.conf");

			FunctionList functionsEGL = new FunctionList();
			
			functionsEGL.AddRange(functionseglh);

			Console.WriteLine("After merging EGL {0}", functionsEGL.Count);
			
			functionsfinal.Clear();
			functionsfinal.AddRange(functionsEGL);
			
			functionsfinal.CorrectionForArrayArguments();
			functionsfinal.CalculateRegisters();
			functionsfinal.ReorderToMatch(orderedExistingFunctionsEGL);			

			MangleFileWriter eglmfw = new MangleFileWriter();
			eglmfw.Write(@"/data/deadwood/temp/egl_mangle.h", functionsfinal);

			MangledHeaderFileWriter eglmhfw = new MangledHeaderFileWriter();
			eglmhfw.Write(@"/data/deadwood/temp/eglapim.h", functionsfinal);

			StubsFileWriter eglsfw = new StubsFileWriter(false, "EGL", 35);
			eglsfw.Write(@"/data/deadwood/temp/egl_library_api.c", functionsfinal);
			
			ConfFileWriter eglcfw = new ConfFileWriter();
			eglcfw.Write(@"/data/deadwood/temp/egl.conf", functionsfinal);


			/* VG */
			FunctionList functionsopenvgh = p.Parse(PATH_TO_MESA + @"/include/VG/openvg.h", APIHeaderParser.VGAPI, APIHeaderParser.VGAPIENTRY);
			FunctionList functionsvguh = p.Parse(PATH_TO_MESA + @"/include/VG/vgu.h", APIHeaderParser.VGUAPI, APIHeaderParser.VGUAPIENTRY);

			FunctionList orderedExistingFunctionsVG = confParser.Parse(PATH_TO_MESA + @"/src/aros/vega/vega.conf");
			
			FunctionList functionsVG = new FunctionList();
			functionsVG.AddRange(functionsopenvgh);
			functionsVG.AddRange(functionsvguh);
			functionsVG.RemoveFunctionByName("vguComputeWarpQuadToQuad"); /* Too many parameters */
			
			Console.WriteLine("After merging VG {0}", functionsVG.Count);
			
			functionsfinal.Clear();
			functionsfinal.AddRange(functionsVG);
			
			functionsfinal.CorrectionForArrayArguments();
			functionsfinal.CalculateRegisters();
			functionsfinal.ReorderToMatch(orderedExistingFunctionsVG);			

			MangleFileWriter vgmfw = new MangleFileWriter();
			vgmfw.Write(@"/data/deadwood/temp/vg_mangle.h", functionsfinal);

			MangledHeaderFileWriter vgmhfw = new MangledHeaderFileWriter();
			vgmhfw.Write(@"/data/deadwood/temp/vgapim.h", functionsfinal);

			StubsFileWriter vgsfw = new StubsFileWriter(false, "Vega", 35);
			vgsfw.Write(@"/data/deadwood/temp/vega_library_api.c", functionsfinal);
			
			ConfFileWriter vgcfw = new ConfFileWriter();
			vgcfw.Write(@"/data/deadwood/temp/vega.conf", functionsfinal);


			/* GLU */
			FunctionList functionsgluh = p.Parse(PATH_TO_MESA + @"/include/GL/glu.h", APIHeaderParser.GLAPI, APIHeaderParser.GLAPIENTRY);

			FunctionList orderedExistingFunctionsGLU = confParser.Parse(PATH_TO_MESA + @"/src/aros/glu/glu.conf");
			
			FunctionList functionsGLU = new FunctionList();
			functionsGLU.AddRange(functionsgluh);
			functionsGLU.RemoveFunctionByName("gluUnProject4"); /* Too many parameters */
			
			Console.WriteLine("After merging GLU {0}", functionsGLU.Count);
			
			functionsfinal.Clear();
			functionsfinal.AddRange(functionsGLU);
			
			functionsfinal.CorrectionForArrayArguments();
			functionsfinal.CalculateRegisters();
			functionsfinal.ReorderToMatch(orderedExistingFunctionsGLU);			

			MangleFileWriter glumfw = new MangleFileWriter();
			glumfw.Write(@"/data/deadwood/temp/glu_mangle.h", functionsfinal);

			MangledHeaderFileWriter glumhfw = new MangledHeaderFileWriter();
			glumhfw.Write(@"/data/deadwood/temp/gluapim.h", functionsfinal);

			StubsFileWriter glusfw = new StubsFileWriter(false, "GLU", 35);
			glusfw.Write(@"/data/deadwood/temp/glu_library_api.c", functionsfinal);
			
			ConfFileWriter glucfw = new ConfFileWriter();
			glucfw.Write(@"/data/deadwood/temp/glu.conf", functionsfinal);

		}
	}
}