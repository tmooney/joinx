#!/usr/bin/env python

from integrationtest import IntegrationTest, main
import unittest

class TestSort(IntegrationTest, unittest.TestCase):

    def test_sort_bed(self):
        input_files = self.inputFiles("sort/unsorted*.bed")
        expected_file = self.inputFiles("sort/expected-sort.bed")[0]
        output_file = self.tempFile("output.bed")

        # test normal and stable sort
        for arg in ["", "-s"]:
            params = [ "sort", "-o", output_file, arg ]
            params.extend(input_files)
            rv, err = self.execute(params)
            self.assertEqual(0, rv)
            self.assertEqual('', err)
            self.assertFilesEqual(expected_file, output_file)

    def test_sort_compressed_bed(self):
        input_files = self.inputFiles("sort/unsorted*.bed.gz")
        expected_file = self.inputFiles("sort/expected-sort.bed")[0]
        output_file = self.tempFile("output.bed")

        # test normal and stable sort
        for arg in ["", "-s"]:
            params = [ "sort", "-o", output_file, arg ]
            params.extend(input_files)
            rv, err = self.execute(params)
            self.assertEqual(0, rv)
            self.assertEqual('', err)
            self.assertFilesEqual(expected_file, output_file)

    def test_sort_unique(self):
        output_file = self.tempFile("output.bed")
        params = [ "sort", "-u", "-o", output_file ]
        params.extend(self.inputFiles(
            "sort/union-a.bed",
            "sort/union-b.bed"
        ))
        rv, err = self.execute(params)
        self.assertEqual('', err)
        self.assertEqual(0, rv)
        expected_file = self.inputFiles("sort/expected-union.bed")[0]
        self.assertFilesEqual(expected_file, output_file)

    def test_sort_tmp(self):
        input_files = self.inputFiles("sort/unsorted*.bed")
        expected_file = self.inputFiles("sort/expected-sort.bed")[0]
        output_file = self.tempFile("output.bed")

        lines = sum([len(open(x).readlines()) for x in input_files])
        max_lines = lines / 10

        # test normal and stable sort
        params = [ "sort", "-M", str(max_lines), "-o", output_file ]
        params.extend(input_files)
        rv, err = self.execute(params)
        self.assertEqual(0, rv)
        self.assertEqual('', err)
        self.assertFilesEqual(expected_file, output_file)

    def test_sort_compressed_vcf(self):
        input_files = self.inputFiles("sort/unsorted*.vcf.gz")
        expected_file = self.inputFiles("sort/expected-sort.vcf")[0]
        output_file = self.tempFile("output.bed")
        params = [ "sort", "-o", output_file ]
        params.extend(input_files)
        rv, err = self.execute(params)
        self.assertEqual('', err)
        self.assertEqual(0, rv)
        self.assertFilesEqual(expected_file, output_file, filter_regex="##fileDate=")

    def test_sort_vcf(self):
        # currently only 1 vcf file at a time can be sorted, as it is trickier
        # to merge vcf than bed.
        input_files = self.inputFiles("sort/unsorted*.vcf")
        expected_file = self.inputFiles("sort/expected-sort.vcf")[0]
        output_file = self.tempFile("output.bed")
        params = [ "sort", "-o", output_file ]
        params.extend(input_files)
        rv, err = self.execute(params)
        self.assertEqual('', err)
        self.assertEqual(0, rv)
        self.assertFilesEqual(expected_file, output_file, filter_regex="##fileDate=")

    def test_sort_some_empty(self):
        expected_file = self.inputFiles("sort/expected-sort.bed")[0]
        input_files = self.inputFiles("sort/unsorted*.bed")
        temp_empty = self.tempFile("empty.bed")
        open(temp_empty, "w").close();
        input_files.append(temp_empty)
        output_file = self.tempFile("output.bed")
        params = [ "sort", "-o", output_file ]
        params.extend(input_files)
        rv, err = self.execute(params)
        self.assertEqual(0, rv, err)
        self.assertEqual('', err)
        self.assertFilesEqual(expected_file, output_file)

    def test_sort_all_empty(self):
        expected_file = temp_empty = self.tempFile("empty.bed")
        open(temp_empty, "w").close();
        input_files = [(temp_empty)]
        output_file = self.tempFile("output.bed")
        params = [ "sort", "-o", output_file ]
        params.extend(input_files)
        rv, err = self.execute(params)
        self.assertEqual(0, rv, err)
        self.assertEqual('', err)
        self.assertFilesEqual(expected_file, output_file)


if __name__ == "__main__":
    main()

