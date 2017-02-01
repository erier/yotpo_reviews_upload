<?php
function yotpo_reviews_upload_config_form_validate($form, &$form_state) {
	$validators = array('file_validate_extensions' => array('csv'));

	// Check for a new uploaded favicon.
	$file = file_save_upload('yotpo_csv', $validators);
	if (isset($file)) {
	// File upload was attempted.
		if ($file) {
			// Put the temporary file in form_values so we can save it on submit.
			$form_state['input']['yotpo_csv'] = $file;

		} else {
			// File upload failed.
			form_set_error('yotpo_csv', t('The CSV could not be uploaded.'));
		}
	}
}

function yotpo_reviews_upload_config_form($form, &$form_stat) {
	$form = array();

	$form['intro'] = array(
		'#markup' => '<h1>Yotpo Review CSV Upload</h1><p>Yotpo provides access to a CSV of your reviews (site and product). For help on finding out how to get the CSV, reference this <a href="https://support.yotpo.com/en/article/exporting-reviews-from-yotpo" target="_blank">document</a>.</p><p>Once you\'ve downloaded your feed, use this tool to upload the file. The contents of the CSV will be ported to a dedicated table in your site database.</p>'
	);

	$form['yotpo_csv'] = array(
		'#type' => 'managed_file',
		'#name' => 'yotpo_csv',
		'#title' => 'Yotpo Review CSV',
		'#upload_location' => 'private://yotpo-reviews/',
		'#upload_validators' => array('file_validate_extensions' => array('csv')),
	);

	$form['save_tags'] = array(
		'#type' => 'submit',
		'#value' => 'Save File to Server',
		'#button_type' => ' btn-brl btn-md btn-orange btn-block',
		'#attributes' => array('style' => 'margin-top: 15px; margin-bottom: 15px;')
	);

	return $form;
}

function yotpo_reviews_upload_config_form_submit($form, &$form_state) {
	$file = file_load($form_state['values']['yotpo_csv']);

	if($file) {
		$file->status = FILE_STATUS_PERMANENT;
		file_save($file);

		drupal_set_message('File Saved!');
	} else {
		drupal_set_message('File not saved.');
	}

	yotpoCSVconverter();

	drupal_goto('admin/config/content/yotpo-reviews-upload');
}

/*
latestYotpoFeed()
Takes the latest uploaded Yotpo reviews feed/CSV and
regenerates table 'yotpo_reivews' from the new data
*/
function latestYotpoFeed() {
	$privateFilesPath = variable_get('file_private_path') . '/yotpo-reviews/';

	$files = glob($privateFilesPath . '*.csv');

	$fileArray = array();
	foreach($files as $file) {
		$fileArray[] = array(filemtime($file), $file);
	}

	$key = array_search(max($fileArray), $fileArray);

	$latestFile = $fileArray[$key][1];

	return $latestFile;
}

/*
yotpoCSVconverter()
Takes the latest uploaded Yotpo reviews feed/CSV and
regenerates table 'brl_yotpo_reivews' from the new data
*/
function yotpoCSVconverter() {
	// Truncate the table 'brl_yotpo_reviews'
	db_truncate('yotpo_reviews')->execute();

	// get Latest Yotpo Feed File Uploaded to server
	$latestFile = latestYotpoFeed();

	// Full path to latest file is: $privateFilesPath . '/' . $latestFile[0]
	$handle = fopen($latestFile, 'r');

	$row = fgetcsv($handle);
	$columns = array();
	foreach ($row as $i => $header) {
		// Trim the header of CSV feed
		$columns[$i] = trim($header);
	}

	for ($e = 0; $row = fgetcsv($handle); $e++) {
		$record = array();
		foreach ($row as $field) {
			$record[] = $field;
		}
		// For each row of the CSV, execute db_insert
		db_insert('yotpo_reviews')
		->fields(array(
			'appkey' => $record[0],
			'published' => $record[1],
			'review_title' => $record[2],
			'review_content' => $record[3],
			'review_score' => $record[4],
			'date' => $record[5],
			'user_type' => $record[6],
			'product_id' => $record[7],
			'product_url' => $record[8],
			'product_title' => $record[9],
			'product_description' => $record[10],
			'product_image_url' => $record[11],
			'display_name' => $record[12],
			'email' => $record[13],
			'comment_content' => $record[14],
			'comment_public' => $record[15],
			'comment_created_at' => $record[16]
		))
		->execute();
	}

	fclose($handle);
}