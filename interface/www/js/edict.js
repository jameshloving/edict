function valid_timestamp(value)
{
    return validator.isISO8601(value);
}

function valid_port(value)
{
    return validator.isInt(value, {min:0, max:65535});
}

function valid_address(value)
{
    return validator.isIP(value);
}

function validate(id, value)
{
    if (id == 'form-in-port')
        return valid_port(value);
    if (id == 'form-in-address')
        return valid_address(value);
    if (id == 'form-in-timestamp')
        return valid_timestamp(value);

    return false;
}

$(document).ready(function()
{
    $('#form-in-timestamp').datetimepicker(
    {
        dateFormat: 'yy-mm-dd',
        controlType: 'select',
        timeFormat: 'HH:mm:ssZ',   
    });

    $('[id^=form-in-]').focusout(function()
    {
        console.log($(this).attr('id'));
        console.log($(this).val());
        console.log(validate($(this).attr('id'), $(this).val()));
        if (validate($(this).attr('id'), $(this).val()))
        {
            $(this).removeClass('has-error');
            $(this).addClass('has-success');
            if ($(this).attr('id') != 'form-in-timestamp')
            {
                $(this).parent().addClass('has-success');
                $(this).parent().removeClass('has-error');
            }
            else
            {
                $(this).parent().parent().addClass('has-success');
                $(this).parent().parent().removeClass('has-error');
            }
        }
        else if (validator.trim($(this).val()) != '')
        {
            $(this).removeClass('has-success');
            $(this).addClass('has-error');
            if ($(this).attr('id') != 'form-in-timestamp')
            {
                $(this).parent().addClass('has-error');
                $(this).parent().removeClass('has-success');
            }
            else
            {
                $(this).parent().parent().addClass('has-error');
                $(this).parent().parent().removeClass('has-success');
            }
        }

        if ($('#form-in-timestamp').hasClass('has-success') &&
            $('#form-in-port').hasClass('has-success') &&
            $('#form-in-address').hasClass('has-success'))
        {
            $('#form-submit').removeClass('disabled'); 
            $('#form-submit').removeAttr('disabled');
        }
        else
        {
            $('#form-submit').addClass('disabled');
            $('#form-submit').prop('disabled',true);
        }
    });    

});
