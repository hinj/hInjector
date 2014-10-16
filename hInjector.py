import xml.dom.minidom as dom
import os
import sys

def load_hnum(fileName):
  nums = dom.parse(fileName)
  global mmap
  for hnums in nums.firstChild.childNodes:
    if hnums.nodeName == "hnum":
      mmap.append((hnums.getAttribute("name"), hnums.firstChild.data.strip()))

def _read_nodes(nodes):
  return nodes.firstChild.data.strip()

def _read_params(nodes):
  ret = ""
  if nodes.nodeName == "value":
    key = _read_nodes(nodes)
    ret = ret + "" + key
  if nodes.nodeName == "regular":
    ret = ret + "r"
    for valrange in nodes.childNodes:
      if valrange.nodeName == "value":
        value = _read_nodes(valrange)
        ret = ret + "-" + value
      if valrange.nodeName == "range":
        ret = ret + "-r"
        for lowup in valrange.childNodes:
          if lowup.nodeName == "lower":
            ret = ret + "-" + _read_nodes(lowup)
          if lowup.nodeName == "upper":
            ret = ret + "-" + _read_nodes(lowup)
  return ret

def _read_params_all(parameter):
  ret = "/"
  
  for nodes in parameter:
    if nodes.nodeName == "struct":
      ret = ret + "!" + nodes.getAttribute("name")
      for members in nodes.childNodes:
        if members.nodeName == "member":
          ret = ret + "!" + members.getAttribute("name") + "!"
          for value in members.childNodes:
            ret = ret + _read_params(value)
    elif nodes.nodeName == "regular" or nodes.nodeName == "value":
      ret = ret + _read_params(nodes)
  return ret

def load_scenario(fileName):
  ret = ""
  tree = dom.parse(fileName)
  global mmap

  ret = ret + tree.firstChild.getAttribute("rate")
  ret = ret + ":" + tree.firstChild.getAttribute("repeat")

  for entry in tree.firstChild.childNodes:
    if entry.nodeName == "hcall":
      if ret != "":
        ret = ret + " "
      ret = ret + entry.getAttribute("repeat") + ":"

      for v in mmap:
        if v[0] == entry.getAttribute("name"):
          ret = ret + v[1]

      for paras in entry.childNodes:
        if paras.nodeName == "parameter":
          ret = ret + _read_params_all(paras.childNodes)
  return ret;

mmap = []
load_hnum("./hnums.xml")
try:
  ret = load_scenario("./config/" + sys.argv[1])
except IndexError:
  ret = load_scenario("./config/default.xml")
except IOError:
  print "Cannot find configuration file."
  sys.exit(0)
os.system("./hInjSender " + ret)
