# Converts --fmt string to instance of given archive.
class ArchiveFactory
  def self.get(fmt)
    factory[fmt].call
  end

  def self.format_strings
    factory.keys
  end

  def self.each(&block)
    factory.each { |k, v| block.call(k, v.call) }
  end

  def self.factory
    {
      'xp3' => lambda do
        require 'lib/formats/arc/xp3_archive'
        Xp3Archive
      end,

      'ykc' => lambda do
        require 'lib/formats/arc/ykc_archive'
        YkcArchive
      end,

      'sar' => lambda do
        require 'lib/formats/arc/sar_archive'
        SarArchive
      end,

      'nsa' => lambda do
        require 'lib/formats/arc/nsa_archive'
        NsaArchive
      end,

      'melty_blood' => lambda do
        require 'lib/formats/arc/melty_blood_archive'
        MeltyBloodArchive
      end,

      'nitroplus/pak2' => lambda do
        require 'lib/formats/arc/pak2_nitroplus_archive'
        Pak2NitroplusArchive
      end,

      'fjsys' => lambda do
        require 'lib/formats/arc/fjsys_archive'
        FjsysArchive
      end,

      'npa' => lambda do
        require 'lib/formats/arc/npa_archive'
        NpaArchive
      end,

      'rpa' => lambda do
        require 'lib/formats/arc/rpa_archive'
        RpaArchive
      end,

      'mbl' => lambda do
        require 'lib/formats/arc/mbl_archive'
        MblArchive
      end,

      'exe' => lambda do
        require 'lib/formats/arc/exe_reader'
        ExeReader
      end
    }
  end
end
