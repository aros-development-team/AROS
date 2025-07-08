#!/usr/bin/env python3

import requests
from bs4 import BeautifulSoup
import re
import sys

BASE_URL = 'https://sourceforge.net/projects/arosdev/files/nightly2/'

# Color class to mimic original color blending and hex output
class Color:
    def __init__(self, r, g, b):
        self.r = r
        self.g = g
        self.b = b

    def to_hex(self):
        return f'#{self.r:02X}{self.g:02X}{self.b:02X}'

    @staticmethod
    def blend(c1, c2, max_val, val):
        r = int(c1.r + (c2.r - c1.r) * (val / max_val))
        g = int(c1.g + (c2.g - c1.g) * (val / max_val))
        b = int(c1.b + (c2.b - c1.b) * (val / max_val))
        return Color(r, g, b)

colorBG      = Color(0xEE, 0xEE, 0xEE)
colorFileDL  = Color(0x99, 0xDD, 0x99)
colorFileNA  = Color(0xDC, 0x99, 0x99)
colorTH      = Color(0x91, 0xAB, 0xC6)
colorFrame   = Color(0x00, 0x00, 0x00)
colorGhosted = Color(0x99, 0x77, 0x77)

def get_folder_listing(url):
    response = requests.get(url)
    if not response.ok:
        print(f"Failed to retrieve {url}", file=sys.stderr)
        sys.exit(1)
    soup = BeautifulSoup(response.text, 'html.parser')
    links = soup.select('a')
    folders = []
    for a in links:
        text = a.text.strip()
        if re.fullmatch(r'\d{8}', text):
            folders.append(text)
    folders.sort(reverse=True)
    return folders

def get_packages_for_date(date):
    url = f"{BASE_URL}{date}/"
    response = requests.get(url)
    if not response.ok:
        print(f"Failed to retrieve {url}", file=sys.stderr)
        return {}

    soup = BeautifulSoup(response.text, 'html.parser')
    links = soup.select('a')
    groups = {}

    for a in links:
        href = a.get('href', '')
        text = a.text.strip()

        if text in ('../', 'Parent Directory'):
            continue

        if href.endswith('/'):
            group_name = text.rstrip('/')
            if group_name == 'logs':
                continue
            groups[group_name] = []

            group_url = f"{url}{group_name}/"
            r2 = requests.get(group_url)
            if not r2.ok:
                continue
            soup2 = BeautifulSoup(r2.text, 'html.parser')
            for a2 in soup2.select('a'):
                fname = a2.text.strip()
                if fname in ('../', 'Parent Directory'):
                    continue
                if fname.endswith('.md5'):
                    continue
                if not (fname.endswith('.tar.bz2') or fname.endswith('.zip') or fname.endswith('.lha') or fname.endswith('.php')):
                    continue

                pkgname = fname
                pkgname = re.sub(r'^AROS-\d{8}-', '', pkgname)
                for ext in ['.tar.bz2', '.zip', '.lha', '.php']:
                    if pkgname.endswith(ext):
                        pkgname = pkgname[:-len(ext)]

                groups[group_name].append(pkgname)

    return groups

dates = get_folder_listing(BASE_URL)
span = len(dates)

groups_packages_dates = {}
for date in dates:
    groups = get_packages_for_date(date)
    for group, pkgs in groups.items():
        if group not in groups_packages_dates:
            groups_packages_dates[group] = {}
        for pkg in pkgs:
            if pkg not in groups_packages_dates[group]:
                groups_packages_dates[group][pkg] = []
            groups_packages_dates[group][pkg].append(date)

html = []

# Start full HTML document
html.append("""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<style>
  body {{ font-family: Verdana, Arial, sans-serif; background-color: #EEE; margin: 10px; }}
  table.layout.nightly {{ border-collapse: collapse; width: 100%; }}
  table.layout.nightly td, table.layout.nightly th {{ border: 1px solid black; padding: 4px; }}
  .layout.nightly {{ background-color: #EEE; }}
  .layout.nightly a {{ text-decoration: none; color: black; font-weight: bold; }}
  .layout.nightly a:hover {{ text-decoration: underline; color: #007700; }}
  .layout.padded {{ padding: 6px; }}
  .layout.invis {{ background-color: transparent; border: none; }}
  .layout.nightly p {{ margin: 4px 0; }}
  .layout.nightly font[size="-1"] {{ color: #555; }}
</style>
</head>
<body>
""")

html.append("""<table class="layout nightly" border="0" cellpadding="2" cellspacing="1" bgcolor="{frame_color}">
  <tr class="layout nightly" bgcolor="{th_color}">
    <td class="layout invis" width="35%" rowspan="2"></td>
    <td class="layout padded" align="center" colspan="{span}"><b>Date</b></td>
  </tr>
  <tr class="layout" bgcolor="{th_color}">
""".format(frame_color=colorFrame.to_hex(), th_color=colorTH.to_hex(), span=span))

# Date header row
column = span
for date in dates:
    color = Color.blend(colorFileDL, colorBG, span, column)
    year, month, day = date[:4], date[4:6], date[6:8]
    html.append(f'<td class="layout nightly" align="center" bgcolor="{color.to_hex()}"><b>{year}-{month}-{day}</b></td>')
    column -= 1
html.append("</tr>")

# Main table content
for group in sorted(groups_packages_dates.keys()):
    # Check if this group has any packages with at least one date
    has_content = False
    for package, pkg_dates in groups_packages_dates[group].items():
        if pkg_dates:
            has_content = True
            break
    if not has_content:
        # Skip this group entirely
        continue

    # Print group header row spanning all columns (package + dates)
    html.append(f'<tr class="layout nightly" bgcolor="{colorBG.to_hex()}">')
    html.append(f'<td class="layout padded" align="left" colspan="{1 + len(dates)}"><b>{group}</b></td>')
    html.append('</tr>')

    # Print packages with downloads
    for package in sorted(groups_packages_dates[group].keys()):
        pkg_dates = groups_packages_dates[group][package]
        if not pkg_dates:
            # Skip packages with no downloads at all
            continue
        html.append(f'<tr class="layout nightly" valign="middle" bgcolor="{colorFileDL.to_hex()}">')
        html.append(f'<td class="layout padded" valign="top"><p><i>{package}</i></p><p><font size="-1">No description</font></p></td>')
        for date in dates:
            if date in pkg_dates:
                # Construct your actual download URL here:
                # Example URL structure used in your sample:
                # https://sourceforge.net/projects/arosdev/files/nightly2/{date}/{group}/{filename}/download
                filename = f"AROS-{date.replace('-', '')}-{package}.tar.bz2"
                url_date = date.replace('-', '')
                url = f"https://sourceforge.net/projects/arosdev/files/nightly2/{url_date}/{group}/{filename}/download"
                html.append(f'<td class="layout nightly" align="center" bgcolor="{colorFileDL.to_hex()}"><a href="{url}" target="_blank">Download</a></td>')
            else:
                html.append(f'<td class="layout nightly" align="center" bgcolor="{colorFileNA.to_hex()}"><font color="{colorGhosted.to_hex()}">not available</font></td>')
        html.append('</tr>')

html.append('</table>')

# Join HTML lines
output_html = "\n".join(html)

# Print or save output as needed
print(output_html)
