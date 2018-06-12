import os
import sys
import shutil

def main():
  brave_extension_dir = os.path.realpath(os.path.dirname(
      os.path.dirname(os.path.realpath(__file__))))
  brave_extension_browser_resources_dir = os.path.join(brave_extension_dir, 'dev')
  locales_src_dir_path = brave_extension_browser_resources_dir;
  brave_out_dir = sys.argv[1]
  locales_dest_dir_path = os.path.join(brave_out_dir, 'minter-miner-vpn')
  copy_locales(locales_src_dir_path, locales_dest_dir_path)

def copy_locales(locales_src_dir_path, locales_dest_dir_path):
  try:
    locales_dest_path = os.path.join(locales_dest_dir_path, '*')
    shutil.rmtree(locales_dest_path)
  except:
    pass
  #shutil.copytree("d:\\brave-browser\\src\\brave\\vendor\\minter-miner-vpn\\dev\\", "d:\\brave-browser\\src\\brave\\browser\\resources\\minter-miner-vpn\\")
  #src = os.path.normpath("d:\\brave-browser\\src\\brave\\vendor\\minter-miner-vpn\\dev\\")
  #dst = os.path.normpath("d:\\brave-browser\\src\\brave\\browser\\resources\\minter-miner-vpn\\")
  #for item in os.listdir(src):
  #  s = os.path.join(src, item)
  #  d = os.path.join(dst, item)
  #  if os.path.isdir(s):
  #    shutil.copytree(s, d, False, None)
  #  else:
  #    shutil.copy2(s, d)

if __name__ == '__main__':
  sys.exit(main())
