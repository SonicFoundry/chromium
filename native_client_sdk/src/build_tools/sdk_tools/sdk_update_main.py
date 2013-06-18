#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# CMD code copied from git_cl.py in depot_tools.

import config
import cStringIO
import download
import logging
import optparse
import os
import re
import sdk_update_common
from sdk_update_common import Error
import sys
import urllib2

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PARENT_DIR = os.path.dirname(SCRIPT_DIR)

sys.path.append(os.path.dirname(SCRIPT_DIR))
import manifest_util


# Import late so each command script can find our imports
import command.info
import command.list
import command.sources
import command.uninstall
import command.update

# This revision number is autogenerated from the Chrome revision.
REVISION = '{REVISION}'

GSTORE_URL = 'https://commondatastorage.googleapis.com/nativeclient-mirror'
CONFIG_FILENAME = 'naclsdk_config.json'
MANIFEST_FILENAME = 'naclsdk_manifest2.json'
DEFAULT_SDK_ROOT = os.path.abspath(PARENT_DIR)
USER_DATA_DIR = os.path.join(DEFAULT_SDK_ROOT, 'sdk_cache')


def usage(more):
  def hook(fn):
    fn.usage_more = more
    return fn
  return hook


def hide(fn):
  fn.hide = True
  return fn


def LoadConfig(raise_on_error=False):
  path = os.path.join(USER_DATA_DIR, CONFIG_FILENAME)
  cfg = config.Config()
  if not os.path.exists(path):
    return cfg

  try:
    try:
      with open(path) as f:
        file_data = f.read()
    except IOError as e:
      raise Error('Unable to read config from "%s".\n  %s' % (path, e))

    try:
      cfg.LoadJson(file_data)
    except Error as e:
      raise Error('Parsing config file from "%s" failed.\n  %s' % (path, e))
    return cfg
  except Error as e:
    if raise_on_error:
      raise
    else:
      logging.warn(str(e))

  return cfg


def WriteConfig(cfg):
  path = os.path.join(USER_DATA_DIR, CONFIG_FILENAME)
  try:
    sdk_update_common.MakeDirs(USER_DATA_DIR)
  except Exception as e:
    raise Error('Unable to create directory "%s".\n  %s' % (USER_DATA_DIR, e))

  cfg_json = cfg.ToJson()

  try:
    with open(path, 'w') as f:
      f.write(cfg_json)
  except IOError as e:
    raise Error('Unable to write config to "%s".\n  %s' % (path, e))


def LoadLocalManifest(raise_on_error=False):
  path = os.path.join(USER_DATA_DIR, MANIFEST_FILENAME)
  manifest = manifest_util.SDKManifest()
  try:
    try:
      with open(path) as f:
        manifest_string = f.read()
    except IOError as e:
      raise Error('Unable to read manifest from "%s".\n  %s' % (path, e))

    try:
      manifest.LoadDataFromString(manifest_string)
    except Exception as e:
      raise Error('Parsing local manifest "%s" failed.\n  %s' % (path, e))
  except Error as e:
    if raise_on_error:
      raise
    else:
      logging.warn(str(e))
  return manifest


def WriteLocalManifest(manifest):
  path = os.path.join(USER_DATA_DIR, MANIFEST_FILENAME)
  try:
    sdk_update_common.MakeDirs(USER_DATA_DIR)
  except Exception as e:
    raise Error('Unable to create directory "%s".\n  %s' % (USER_DATA_DIR, e))

  try:
    manifest_json = manifest.GetDataAsString()
  except Exception as e:
    raise Error('Error encoding manifest "%s" to JSON.\n  %s' % (path, e))

  try:
    with open(path, 'w') as f:
      f.write(manifest_json)
  except IOError as e:
    raise Error('Unable to write manifest to "%s".\n  %s' % (path, e))


def LoadRemoteManifest(url):
  manifest = manifest_util.SDKManifest()
  url_stream = None
  try:
    manifest_stream = cStringIO.StringIO()
    url_stream = download.UrlOpen(url)
    download.DownloadAndComputeHash(url_stream, manifest_stream)
  except urllib2.URLError as e:
    raise Error('Unable to read remote manifest from URL "%s".\n  %s' % (
        url, e))
  finally:
    if url_stream:
      url_stream.close()

  try:
    manifest.LoadDataFromString(manifest_stream.getvalue())
    return manifest
  except manifest_util.Error as e:
    raise Error('Parsing remote manifest from URL "%s" failed.\n  %s' % (
        url, e,))


def LoadCombinedRemoteManifest(default_manifest_url, cfg):
  manifest = LoadRemoteManifest(default_manifest_url)
  for source in cfg.sources:
    manifest.MergeManifest(LoadRemoteManifest(source))
  return manifest


# Commands #####################################################################


@usage('<bundle names...>')
def CMDinfo(parser, args):
  """display information about a bundle"""
  options, args = parser.parse_args(args)
  if not args:
    parser.error('No bundles given')
    return 0
  cfg = LoadConfig()
  remote_manifest = LoadCombinedRemoteManifest(options.manifest_url, cfg)
  command.info.Info(remote_manifest, args)
  return 0


def CMDlist(parser, args):
  """list all available bundles"""
  parser.add_option('-r', '--revision', action='store_true',
                    help='display revision numbers')
  options, args = parser.parse_args(args)
  if args:
    parser.error('Unsupported argument(s): %s' % ', '.join(args))
  local_manifest = LoadLocalManifest()
  cfg = LoadConfig()
  remote_manifest = LoadCombinedRemoteManifest(options.manifest_url, cfg)
  command.list.List(remote_manifest, local_manifest, options.revision)
  return 0


@usage('<bundle names...>')
def CMDupdate(parser, args):
  """update a bundle in the SDK to the latest version"""
  parser.add_option('-F', '--force', action='store_true',
      help='Force updating bundles that already exist. The bundle will not be '
          'updated if the local revision matches the remote revision.')
  options, args = parser.parse_args(args)
  local_manifest = LoadLocalManifest()
  cfg = LoadConfig()
  remote_manifest = LoadCombinedRemoteManifest(options.manifest_url, cfg)

  if not args:
    args = [command.update.RECOMMENDED]

  try:
    delegate = command.update.RealUpdateDelegate(USER_DATA_DIR,
                                                 DEFAULT_SDK_ROOT, cfg)
    command.update.Update(delegate, remote_manifest, local_manifest, args,
                          options.force)
  finally:
    # Always write out the local manifest, we may have successfully updated one
    # or more bundles before failing.
    try:
      WriteLocalManifest(local_manifest)
    except Error as e:
      # Log the error writing to the manifest, but propagate the original
      # exception.
      logging.error(str(e))

  return 0


def CMDinstall(parser, args):
  """install a bundle in the SDK"""
  # For now, forward to CMDupdate. We may want different behavior for this
  # in the future, though...
  return CMDupdate(parser, args)


@usage('<bundle names...>')
def CMDuninstall(parser, args):
  """uninstall the given bundles"""
  _, args = parser.parse_args(args)
  if not args:
    parser.error('No bundles given')
    return 0
  local_manifest = LoadLocalManifest()
  command.uninstall.Uninstall(DEFAULT_SDK_ROOT, local_manifest, args)
  WriteLocalManifest(local_manifest)
  return 0


@usage('<bundle names...>')
def CMDreinstall(parser, args):
  """restore the given bundles to their original state

  Note that if there is an update to a given bundle, reinstall will not
  automatically update to the newest version.
  """
  _, args = parser.parse_args(args)
  local_manifest = LoadLocalManifest()

  if not args:
    parser.error('No bundles given')
    return 0

  cfg = LoadConfig()
  try:
    delegate = command.update.RealUpdateDelegate(USER_DATA_DIR,
                                                 DEFAULT_SDK_ROOT, cfg)
    command.update.Reinstall(delegate, local_manifest, args)
  finally:
    # Always write out the local manifest, we may have successfully updated one
    # or more bundles before failing.
    try:
      WriteLocalManifest(local_manifest)
    except Error as e:
      # Log the error writing to the manifest, but propagate the original
      # exception.
      logging.error(str(e))

  return 0


def CMDsources(parser, args):
  """manage external package sources"""
  parser.add_option('-a', '--add', dest='url_to_add',
                    help='Add an additional package source')
  parser.add_option(
      '-r', '--remove', dest='url_to_remove',
      help='Remove package source (use \'all\' for all additional sources)')
  parser.add_option('-l', '--list', dest='do_list', action='store_true',
                    help='List additional package sources')
  options, args = parser.parse_args(args)

  cfg = LoadConfig(True)
  write_config = False
  if options.url_to_add:
    command.sources.AddSource(cfg, options.url_to_add)
    write_config = True
  elif options.url_to_remove:
    command.sources.RemoveSource(cfg, options.url_to_remove)
    write_config = True
  elif options.do_list:
    command.sources.ListSources(cfg)
  else:
    parser.print_help()

  if write_config:
    WriteConfig(cfg)

  return 0


def CMDversion(parser, args):
  """display version information"""
  _, _ = parser.parse_args(args)
  print "Native Client SDK Updater, version r%s" % REVISION
  return 0


def CMDhelp(parser, args):
  """print list of commands or help for a specific command"""
  _, args = parser.parse_args(args)
  if len(args) == 1:
    return main(args + ['--help'])
  parser.print_help()
  return 0


def Command(name):
  return globals().get('CMD' + name, None)


def GenUsage(parser, cmd):
  """Modify an OptParse object with the function's documentation."""
  obj = Command(cmd)
  more = getattr(obj, 'usage_more', '')
  if cmd == 'help':
    cmd = '<command>'
  else:
    # OptParser.description prefer nicely non-formatted strings.
    parser.description = re.sub('[\r\n ]{2,}', ' ', obj.__doc__)
  parser.set_usage('usage: %%prog %s [options] %s' % (cmd, more))


def UpdateSDKTools(options, args):
  """update the sdk_tools bundle"""

  local_manifest = LoadLocalManifest()
  cfg = LoadConfig()
  remote_manifest = LoadCombinedRemoteManifest(options.manifest_url, cfg)

  try:
    delegate = command.update.RealUpdateDelegate(USER_DATA_DIR,
                                                 DEFAULT_SDK_ROOT, cfg)
    command.update.UpdateBundleIfNeeded(
        delegate,
        remote_manifest,
        local_manifest,
        command.update.SDK_TOOLS,
        force=True)
  finally:
    # Always write out the local manifest, we may have successfully updated one
    # or more bundles before failing.
    WriteLocalManifest(local_manifest)
  return 0


def main(argv):
  # Get all commands...
  cmds = [fn[3:] for fn in dir(sys.modules[__name__]) if fn.startswith('CMD')]
  # Remove hidden commands...
  cmds = filter(lambda fn: not getattr(Command(fn), 'hide', 0), cmds)
  # Format for CMDhelp usage.
  CMDhelp.usage_more = ('\n\nCommands are:\n' + '\n'.join([
      '  %-10s %s' % (fn, Command(fn).__doc__.split('\n')[0].strip())
      for fn in cmds]))

  # Create the option parse and add --verbose support.
  parser = optparse.OptionParser()
  parser.add_option(
      '-v', '--verbose', action='count', default=0,
      help='Use 2 times for more debugging info')
  parser.add_option('-U', '--manifest-url', dest='manifest_url',
      default=GSTORE_URL + '/nacl/nacl_sdk/' + MANIFEST_FILENAME,
      metavar='URL', help='override the default URL for the NaCl manifest file')
  parser.add_option('--update-sdk-tools', action='store_true',
                    dest='update_sdk_tools', help=optparse.SUPPRESS_HELP)

  old_parser_args = parser.parse_args
  def Parse(args):
    options, args = old_parser_args(args)
    if options.verbose >= 2:
      loglevel = logging.DEBUG
    elif options.verbose:
      loglevel = logging.INFO
    else:
      loglevel = logging.WARNING

    fmt = '%(levelname)s:%(message)s'
    logging.basicConfig(stream=sys.stdout, level=loglevel, format=fmt)

    # If --update-sdk-tools is passed, circumvent any other command running.
    if options.update_sdk_tools:
      UpdateSDKTools(options, args)
      sys.exit(1)

    return options, args
  parser.parse_args = Parse

  if argv:
    cmd = Command(argv[0])
    if cmd:
      # "fix" the usage and the description now that we know the subcommand.
      GenUsage(parser, argv[0])
      return cmd(parser, argv[1:])

  # Not a known command. Default to help.
  GenUsage(parser, 'help')
  return CMDhelp(parser, argv)


if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv[1:]))
  except Error as e:
    logging.error(str(e))
    sys.exit(1)
