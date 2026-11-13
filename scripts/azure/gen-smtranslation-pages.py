#!/usr/bin/env python3

import os
import re
from collections import defaultdict

# Language mapping for flags
LANGUAGE_MAP = {
    'albanian': 'Albanian',
    'basque': 'Basque',
    'catalan': 'Catalan',
    'croatian': 'Croatian',
    'czech': 'Czech',
    'danish': 'Danish',
    'dutch': 'Dutch',
    'esperanto': 'Esperanto',
    'finnish': 'Finnish',
    'french': 'French',
    'german': 'German',
    'greek': 'Greek',
    'hungarian': 'Hungarian',
    'icelandic': 'Icelandic',
    'italian': 'Italian',
    'maltese': 'Maltese',
    'norwegian': 'Norwegian',
    'piglatin': 'Pig Latin',
    'polish': 'Polish',
    'portuguese': 'Portuguese',
    'portuguese-brazil': 'Portuguese (Brazil)',
    'romanian': 'Romanian',
    'russian': 'Russian',
    'serbian': 'Serbian',
    'slovak': 'Slovak',
    'slovene': 'Slovene',
    'spanish': 'Spanish',
    'swedish': 'Swedish',
    'thai': 'Thai',
    'turkish': 'Turkish'
    # add more as needed
}

LANG_TO_FLAG = {
    'albanian': 'al.png',
    'basque': 'es.png',
    'catalan': 'es.png',
    'croatian': 'hr.png',
    'czech': 'cz.png',
    'danish': 'dk.png',
    'dutch': 'nl.png',
    'esperanto': 'esperanto.png',
    'finnish': 'fi.png',
    'french': 'fr.png',
    'german': 'de.png',
    'greek': 'gr.png',
    'hungarian': 'hu.png',
    'icelandic': 'is.png',
    'italian': 'it.png',
    'maltese': 'mt.png',
    'norwegian': 'no.png',
    'piglatin': 'generic-language.png',  # or empty string if no image
    'polish': 'pl.png',
    'portuguese': 'pt.png',
    'portuguese-brazil': 'br.png',
    'romanian': 'ro.png',
    'russian': 'ru.png',
    'serbian': 'rs.png',
    'slovak': 'sk.png',
    'slovene': 'si.png',
    'spanish': 'es.png',
    'swedish': 'se.png',
    'thai': 'th.png',
    'turkish': 'tr.png',
}

def wrap_flags_html(langs, size_suffix=""):
    valid_langs = sorted(lang for lang in langs if lang in LANGUAGE_MAP)

    imgs = []
    for lang in valid_langs:
        base_flag_file = LANG_TO_FLAG.get(lang)
        if not base_flag_file:
            continue

        # Build the flag filename with optional suffix
        if size_suffix:
            name, ext = base_flag_file.rsplit('.', 1)
            flag_file = f"{name}-{size_suffix}.{ext}"
        else:
            flag_file = base_flag_file

        # Adjust image height if suffix is '16'
        height = "12px" if size_suffix == "16" else "24px"

        imgs.append(
            f"<img src='/images/flags/{flag_file}' alt='{LANGUAGE_MAP[lang]}' "
            f"title='{LANGUAGE_MAP[lang]}' style='height:{height};margin-right:4px;'>"
        )

    lines = [' '.join(imgs[i:i+4]) for i in range(0, len(imgs), 4)]
    return '<br>'.join(lines)

def parse_input_file(input_path):
    modules = {}
    current_module = None

    with open(input_path, "r") as file:
        for line in file:
            line = line.strip()
            if line.startswith("Submodule:"):
                match = re.match(r"Submodule:\s+(.+?)\s+\((.+?)\)", line)
                if match:
                    module_path = match.group(1)
                    repo_url = match.group(2)
                    module_name = os.path.basename(repo_url).removesuffix(".git")
                    current_module = module_name
                    modules[current_module] = {"repo": repo_url, "errors": [], "languages": set(), "submodule_path": module_path}
            elif current_module and line:
                modules[current_module]["errors"].append(line)

                # Extract the filename from the line (first token before ', line' or space)
                file_path = line.split(",", 1)[0].split(" ", 1)[0]
                filename = os.path.basename(file_path)
                lang_name, ext = os.path.splitext(filename)
                modules[current_module]["languages"].add(lang_name.lower())

    return modules

def generate_module_pages(modules, output_dir):
    import html

    os.makedirs(output_dir, exist_ok=True)

    for module, data in modules.items():
        module_file = os.path.join(output_dir, f"{module}.html")
        repo_base_url = data["repo"].removesuffix(".git")
        known_langs = sorted(lang for lang in data["languages"] if lang in LANGUAGE_MAP)

        with open(module_file, "w") as f:
            f.write("<html><head>\n")
            f.write(f"<title>{module} Translation Errors</title>\n")
            f.write('<meta http-equiv="Content-Type" content="text/html; charset=utf-8">\n')
            f.write('<link rel="stylesheet" type="text/css" href="/aros.css?v=1.6">\n')
            f.write('<link rel="stylesheet" type="text/css" href="/print.css?v=1.0" media="print">\n')
            f.write('<link rel="icon" type="image/x-icon" href="/aros.ico">\n')
            f.write(f'<meta name="keywords" content="{module}, AROS translations, localization, translations">\n')
            f.write('<meta name="viewport" content="width=device-width, initial-scale=1">\n')
            f.write("</head>\n")

            f.write("<body>\n")
            f.write("    <header>\n")
            f.write("        <img src=\"/images/toplogo.png?v=1.0\" alt=\"top logo menu\" class=\"leftimage\">\n")
            f.write("        <img src=\"/images/kittymascot.png?v=1.0\" alt=\"kitty mascot\" class=\"rightimage\">\n")
            f.write("        <div class=\"topmenu\">\n")
            f.write("            <a href=\"http://www.aros.org/\">AROS.ORG</a>\n")
            f.write("            <a href=\"http://developers.aros.org/\">Developers</a>\n")
            f.write("            <a href=\"/\">Localization</a>\n")
            f.write("        </div><!-- topmenu -->\n")
            f.write("    </header>\n")

            f.write(f"<h1>Module: {module}</h1>\n")
            f.write(f"<p>Repository: <a href='{data['repo']}'>{data['repo']}</a></p>\n")
            f.write(f"<p><a href='index.html'>Back to Index</a></p>\n")

            if known_langs:
                f.write("<p>Affected Languages: ")
                # Generate flag images with links to anchors
                flags_html = wrap_flags_html(known_langs, "")  # e.g. images only
                # Simple parsing + wrapping images with <a href="#lang">
                # We do string manipulation here instead of using BeautifulSoup to avoid dependencies
                import re
                def wrap_flag_links(html_flags, langs):
                    # This regex matches each <img ...> tag
                    imgs = re.findall(r'(<img [^>]+>)', html_flags)
                    wrapped = []
                    for i, img_tag in enumerate(imgs):
                        # Extract alt text to find language code
                        alt_match = re.search(r'alt=[\'"]([^\'"]+)[\'"]', img_tag)
                        if alt_match:
                            alt_text = alt_match.group(1).lower()
                            if alt_text in langs:
                                wrapped.append(f"<a href='#{alt_text}'>{img_tag}</a>")
                            else:
                                wrapped.append(img_tag)
                        else:
                            wrapped.append(img_tag)
                    return " ".join(wrapped)

                f.write(wrap_flag_links(flags_html, known_langs))
                f.write("</p>\n")

            f.write("<table border='1' cellpadding='5' cellspacing='0' style='border-collapse: collapse;'>\n")
            f.write("<tr><th>File</th><th>Warning/Error</th></tr>\n")

            # Group errors by file
            errors_by_file = defaultdict(list)
            for error in data["errors"]:
                parts = error.split(" ", 1)
                if len(parts) == 2:
                    file_path, error_text = parts
                else:
                    file_path, error_text = error, ""

                rel_path = file_path
                if file_path.startswith(data["submodule_path"] + "/"):
                    rel_path = file_path[len(data["submodule_path"]) + 1:]

                errors_by_file[rel_path].append(error_text)

            # Output grouped errors with anchors for language files
            for rel_path, errors in errors_by_file.items():
                # Use language code as anchor if it matches a known language, else no anchor
                anchor = ""
                for lang in known_langs:
                    if rel_path.startswith(lang):
                        anchor = f" id='{lang}'"
                        break

                file_url = f"{repo_base_url}/blob/master/{rel_path}"
                # Escape HTML in rel_path and error text for safety
                safe_rel_path = html.escape(rel_path)
                f.write(f"<tr{anchor}><td><a href='{file_url}'>{safe_rel_path}</a></td><td>{html.escape(errors[0])}</td></tr>\n")

                for error_text in errors[1:]:
                    f.write(f"<tr><td></td><td>{html.escape(error_text)}</td></tr>\n")

                # Empty row between file groups
                f.write("<tr><td colspan='2' style='background-color:#eee;'>&nbsp;</td></tr>\n")

            f.write("</table>\n")
            f.write("</body></html>\n")

def generate_index_page(modules, output_dir):
    index_path = os.path.join(output_dir, "index.html")
    with open(index_path, "w") as f:
        f.write("<html><head>\n")
        f.write("<title>Submodules with Build Issues and Errors</title>\n")
        f.write('<meta http-equiv="Content-Type" content="text/html; charset=utf-8">\n')
        f.write('<link rel="stylesheet" type="text/css" href="/aros.css?v=1.6">\n')
        f.write('<link rel="stylesheet" type="text/css" href="/print.css?v=1.0" media="print">\n')
        f.write('<link rel="icon" type="image/x-icon" href="/aros.ico">\n')
        f.write('<meta name="keywords" content="AROS translations, localization, translations">\n')
        f.write('<meta name="viewport" content="width=device-width, initial-scale=1">\n')
        f.write("</head>\n")

        f.write("<body>\n")
        f.write("    <header>\n")
        f.write("        <img src=\"/images/toplogo.png?v=1.0\" alt=\"top logo menu\" class=\"leftimage\">\n")
        f.write("        <img src=\"/images/kittymascot.png?v=1.0\" alt=\"kitty mascot\" class=\"rightimage\">\n")
        f.write("        <div class=\"topmenu\">\n")
        f.write("            <a href=\"http://www.aros.org/\">AROS.ORG</a>\n")
        f.write("            <a href=\"http://developers.aros.org/\">Developers</a>\n")
        f.write("            <a href=\"/\">Localization</a>\n")
        f.write("        </div><!-- topmenu -->\n")
        f.write("    </header>\n")

        f.write("<h1>Submodules with Build Issues and Errors</h1>\n")
        f.write("<table border='1' cellpadding='5'>\n")
        f.write("<tr><th>Affected Languages</th><th>Module</th></tr>\n")

        for module, data in sorted(modules.items()):
            known_langs = sorted(lang for lang in data["languages"] if lang in LANGUAGE_MAP)
            flags_html = wrap_flags_html(known_langs, "16") if known_langs else ''

            # Count total errors/warnings
            total_errors = len(data.get("errors", []))

            # Count unique files affected
            files_affected = set()
            for error in data.get("errors", []):
                parts = error.split(" ", 1)
                file_path = parts[0]
                if file_path.startswith(data["submodule_path"] + "/"):
                    file_path = file_path[len(data["submodule_path"]) + 1:]
                files_affected.add(file_path)
            num_files = len(files_affected)

            f.write("<tr>")
            f.write(f"<td align='right'>{flags_html}</td>")
            f.write("<td>")
            f.write(f"<a href='{module}.html'>{module}</a><br>")
            f.write(f"({total_errors} warnings/errors, in {num_files} files)")
            f.write("</td>")
            f.write("</tr>\n")

        f.write("</table>\n")
        f.write("</body></html>\n")

def main():
    import sys

    if len(sys.argv) != 2:
        print("Usage: gen-translation-pages.py <input_file>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_dir = "output"

    modules = parse_input_file(input_file)
    generate_module_pages(modules, output_dir)
    generate_index_page(modules, output_dir)

    print(f"HTML pages generated in: {output_dir}")

if __name__ == "__main__":
    main()
